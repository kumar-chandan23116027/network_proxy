#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <string>
#include <vector>
#include <pthread.h>
#include <netinet/in.h>

// Typedefs for socket compatibility
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

using namespace std;

// Structures
struct ProxyConfig {
    int port;
    int buffer_size;
    int cache_capacity;
    string blacklist_file;
    string cache_path;
};

struct ClientInfo {
    SOCKET socket;
    string ip_address;
};

// Global Variables (Extern) - defined in proxy_server.cpp
extern ProxyConfig config;
extern pthread_mutex_t output_lock;

// Function Declarations
void load_config(string filename);
void load_blacklist(string filename);
void* handle_client(void* args);
void log(string ip, string msg);
string get_timestamp();

#endif