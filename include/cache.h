#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <list>
#include <unordered_map>
#include <pthread.h>

using namespace std;

struct CacheEntry {
    string key;
    string data;
};

class LRUCache {
private:
    int capacity;
    list<CacheEntry> lru_list;
    unordered_map<string, list<CacheEntry>::iterator> cache_map;
    pthread_mutex_t cache_lock;

public:
    LRUCache();
    void setCapacity(int cap);
    ~LRUCache();
    string get(string key);
    void put(string key, string data);
};

#endif