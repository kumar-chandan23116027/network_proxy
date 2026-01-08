#include "../include/proxy_server.h"
#include "../include/cache.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Defaults
#define DEFAULT_PORT 8080
#define DEFAULT_BUFFER_SIZE 8192
#define DEFAULT_CACHE_CAPACITY 10
#define SOCKET_TIMEOUT_SEC 5

using namespace std;

// Define Globals
ProxyConfig config;
pthread_mutex_t output_lock;
vector<string> BLACKLIST;
LRUCache proxy_cache; // Global cache instance internal to this file

// --- Helper Functions ---
string get_timestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tstruct);
    return string(buf);
}

void log(string ip, string msg) {
    if (msg.find("detectportal") != string::npos || msg.find("push.services") != string::npos) return;
    pthread_mutex_lock(&output_lock);
    cout << "[" << get_timestamp() << "] [" << ip << "] " << msg << endl;
    pthread_mutex_unlock(&output_lock);
}

void load_config(string filename) {
    ifstream file(filename);
    config.port = DEFAULT_PORT;
    config.buffer_size = DEFAULT_BUFFER_SIZE;
    config.cache_capacity = DEFAULT_CACHE_CAPACITY;
    config.blacklist_file = "config/blocked_domains.txt"; // Adjusted default path

    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            stringstream ss(line);
            string key;
            if (getline(ss, key, '=')) {
                string value;
                if (getline(ss, value)) {
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    if (key == "port") config.port = stoi(value);
                    else if (key == "buffer_size") config.buffer_size = stoi(value);
                    else if (key == "cache_capacity") {
                        config.cache_capacity = stoi(value);
                        proxy_cache.setCapacity(config.cache_capacity);
                    }
                    else if (key == "blacklist_file") config.blacklist_file = value;
                    else if (key == "cache_path") config.cache_path = value;
                }
            }
        }
        file.close();
        cout << "[SYSTEM] Configuration loaded from " << filename << endl;
    } else {
        cout << "[SYSTEM] Config file not found. Using defaults." << endl;
        proxy_cache.setCapacity(DEFAULT_CACHE_CAPACITY);
    }
}

void load_blacklist(string filename) {
    ifstream file(filename);
    string line;
    BLACKLIST.clear();
    
    if (file.is_open()) {
        while (getline(file, line)) {
            if (!line.empty()) {
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                transform(line.begin(), line.end(), line.begin(), ::tolower);
                if (!line.empty()) BLACKLIST.push_back(line);
            }
        }
        file.close();
        log("SYSTEM", "Blacklist loaded: " + to_string(BLACKLIST.size()) + " domains.");
    } else {
        log("SYSTEM", "WARNING: Could not open " + filename + ". Using minimal defaults.");
        BLACKLIST.push_back("facebook.com");
    }
}

void set_socket_timeout(SOCKET s, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

bool is_blacklisted(string host) {
    string check_host = host;
    transform(check_host.begin(), check_host.end(), check_host.begin(), ::tolower);
    for (const auto& blocked : BLACKLIST) {
        if (check_host.find(blocked) != string::npos) return true;
    }
    return false;
}

void disable_keep_alive(string& req) {
    string lower_req = req;
    transform(lower_req.begin(), lower_req.end(), lower_req.begin(), ::tolower);
    size_t pos = lower_req.find("connection:");
    if (pos != string::npos) {
        size_t end_line = req.find("\r\n", pos);
        if (end_line != string::npos) {
            req.replace(pos, end_line - pos, "Connection: close");
        }
    } else {
        size_t end_headers = req.find("\r\n\r\n");
        if (end_headers != string::npos) {
            req.insert(end_headers, "\r\nConnection: close");
        }
    }
}

// --- Logic Handlers ---

struct RelayArgs { SOCKET s1; SOCKET s2; };

void* relay_thread(void* args) {
    RelayArgs* r = (RelayArgs*)args;
    char* buffer = new char[config.buffer_size];
    while (true) {
        int bytes = recv(r->s1, buffer, config.buffer_size, 0);
        if (bytes <= 0) break;
        send(r->s2, buffer, bytes, 0);
    }
    delete[] buffer;
    delete r;
    return NULL;
}

void handle_https_tunnel(SOCKET client_sock, string host, string port, string client_ip) {
    log(client_ip, "HTTPS Tunneling for: " + host);
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &res) != 0) return;
    
    SOCKET remote_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (remote_sock == INVALID_SOCKET) { freeaddrinfo(res); return; }
    set_socket_timeout(remote_sock, SOCKET_TIMEOUT_SEC);

    if (connect(remote_sock, res->ai_addr, (int)res->ai_addrlen) < 0) {
        freeaddrinfo(res); close(remote_sock); return;
    }
    freeaddrinfo(res);

    string msg = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_sock, msg.c_str(), msg.size(), 0);

    pthread_t t1, t2;
    RelayArgs* args1 = new RelayArgs{client_sock, remote_sock};
    RelayArgs* args2 = new RelayArgs{remote_sock, client_sock};
    pthread_create(&t1, NULL, relay_thread, (void*)args1);
    pthread_create(&t2, NULL, relay_thread, (void*)args2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    close(remote_sock);
}

void handle_http_request(SOCKET client_sock, string cache_key, string host, char* initial_buffer, int initial_len, string client_ip) {
    string cached_data = proxy_cache.get(cache_key);
    if (!cached_data.empty()) {
        log(client_ip, "CACHE HIT: " + host);
        send(client_sock, cached_data.c_str(), cached_data.size(), 0);
        return;
    }

    log(client_ip, "Visiting: " + host);
    struct hostent *server = gethostbyname(host.c_str());
    if (!server) return;

    SOCKET dest_sock = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(dest_sock, SOCKET_TIMEOUT_SEC);

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);
    memcpy((char *)&dest_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);

    if (connect(dest_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        close(dest_sock); return;
    }

    string request_str(initial_buffer, initial_len);
    disable_keep_alive(request_str);
    if (send(dest_sock, request_str.c_str(), request_str.size(), 0) < 0) {
        close(dest_sock); return;
    }

    string full_response = "";
    bool should_cache = true;
    char* temp_buff = new char[config.buffer_size];
    while (true) {
        int n = recv(dest_sock, temp_buff, config.buffer_size, 0);
        if (n <= 0) break;
        send(client_sock, temp_buff, n, 0);
        if (should_cache) {
            if (full_response.size() + n <= 500000) full_response.append(temp_buff, n);
            else { should_cache = false; full_response.clear(); }
        }
    }
    delete[] temp_buff;
    if (should_cache && !full_response.empty()) proxy_cache.put(cache_key, full_response);
    close(dest_sock);
}

void* handle_client(void* args) {
    ClientInfo* info = (ClientInfo*)args;
    SOCKET client_sock = info->socket;
    string client_ip = info->ip_address;
    delete info;

    set_socket_timeout(client_sock, SOCKET_TIMEOUT_SEC);
    char* buffer = new char[config.buffer_size];
    memset(buffer, 0, config.buffer_size);
    int total_bytes = 0;

    while (total_bytes < config.buffer_size - 1) {
        int n = recv(client_sock, buffer + total_bytes, config.buffer_size - total_bytes - 1, 0);
        if (n <= 0) break;
        total_bytes += n;
        if (strstr(buffer, "\r\n\r\n")) break;
    }

    if (total_bytes > 0) {
        string request(buffer, total_bytes);
        string host, port = "80";
        string req_lower = request;
        transform(req_lower.begin(), req_lower.end(), req_lower.begin(), ::tolower);

        size_t host_pos = req_lower.find("host: ");
        if (host_pos != string::npos) {
            size_t end = request.find("\r\n", host_pos);
            host = request.substr(host_pos + 6, end - (host_pos + 6));
        }

        size_t colon_pos = host.find(':');
        if (colon_pos != string::npos) {
            port = host.substr(colon_pos + 1);
            host = host.substr(0, colon_pos);
        }

        if (is_blacklisted(host)) {
            log(client_ip, "BLOCKED: " + host);
            string msg = "HTTP/1.1 403 Forbidden\r\n\r\n<h1>403 Forbidden</h1><p>Blocked by Proxy.</p>";
            send(client_sock, msg.c_str(), msg.size(), 0);
        } else if (request.find("CONNECT") == 0) {
            handle_https_tunnel(client_sock, host, port, client_ip);
        } else {
            size_t fl_end = request.find("\r\n");
            string first_line = (fl_end != string::npos) ? request.substr(0, fl_end) : "UNKNOWN";
            string cache_key = first_line + " | Host: " + host;
            handle_http_request(client_sock, cache_key, host, buffer, total_bytes, client_ip);
        }
    }
    delete[] buffer;
    close(client_sock);
    return NULL;
}