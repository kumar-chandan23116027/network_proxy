#include "../include/cache.h"
#include <iostream>

using namespace std;

LRUCache::LRUCache() {
    pthread_mutex_init(&cache_lock, NULL);
    capacity = 10; // Default fallback
}

void LRUCache::setCapacity(int cap) {
    capacity = cap;
}

LRUCache::~LRUCache() {
    pthread_mutex_destroy(&cache_lock);
}

string LRUCache::get(string key) {
    pthread_mutex_lock(&cache_lock);
    string result = "";
    if (cache_map.find(key) != cache_map.end()) {
        lru_list.splice(lru_list.begin(), lru_list, cache_map[key]);
        result = cache_map[key]->data;
    }
    pthread_mutex_unlock(&cache_lock);
    return result;
}

void LRUCache::put(string key, string data) {
    if (data.size() > 500000) return; // 500KB fixed limit

    pthread_mutex_lock(&cache_lock);
    if (cache_map.find(key) != cache_map.end()) {
        lru_list.splice(lru_list.begin(), lru_list, cache_map[key]);
        cache_map[key]->data = data;
    } else {
        if ((int)lru_list.size() == capacity) {
            string last_key = lru_list.back().key;
            lru_list.pop_back();
            cache_map.erase(last_key);
        }
        lru_list.push_front({key, data});
        cache_map[key] = lru_list.begin();
    }
    pthread_mutex_unlock(&cache_lock);
}