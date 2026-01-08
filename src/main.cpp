#include "../include/proxy_server.h"
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

// State needed for signal handling in main
volatile sig_atomic_t server_running = 1;
SOCKET server_socket = INVALID_SOCKET;

void handle_signal(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        server_running = 0;
        if (server_socket != INVALID_SOCKET) {
            close(server_socket);
            server_socket = INVALID_SOCKET;
        }
    }
}

int main() {
    pthread_mutex_init(&output_lock, NULL);
    signal(SIGPIPE, SIG_IGN); 
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // लोड config/proxy.conf (सही पथ का उपयोग करें)
    load_config("config/proxy.conf");
    load_blacklist(config.blacklist_file);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config.port);

    if (bind(server_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(server_socket, 20);

    cout << "=============================================" << endl;
    cout << "   MASTER PROXY SERVER (Modular Edition)     " << endl;
    cout << "   Listening on Port " << config.port << endl;
    cout << "   Press Ctrl+C to Shutdown Gracefully       " << endl;
    cout << "=============================================" << endl;

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        SOCKET client = accept(server_socket, (struct sockaddr *)&client_addr, &len);
        
        if (server_running && client != INVALID_SOCKET) {
            char* ip = inet_ntoa(client_addr.sin_addr);
            
            ClientInfo* info = new ClientInfo;
            info->socket = client;
            info->ip_address = string(ip);

            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, (void*)info) != 0) {
                log("SYSTEM", "Failed to create thread");
                close(client);
                delete info;
            } else {
                pthread_detach(tid);
            }
        }
    }

    cout << "\n[SYSTEM] Shutting down server..." << endl;
    if (server_socket != INVALID_SOCKET) close(server_socket);
    pthread_mutex_destroy(&output_lock);
    cout << "[SYSTEM] Bye!" << endl;
    return 0;
}