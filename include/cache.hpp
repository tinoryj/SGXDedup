#ifndef GENERALDEDUPSYSTEM_CACHE_HPP
#define GENERALDEDUPSYSTEM_CACHE_HPP

#include "configure.hpp"
#include <bits/stdc++.h>
#include <boost/compute/detail/lru_cache.hpp>
#include <boost/thread.hpp>

extern Configure config;

using namespace boost::compute::detail;
using namespace std;

class keyCache {
private:
    lru_cache<string, string>* lruKeyCache_;
    std::mutex mutexKeyCache_;

public:
    keyCache();
    ~keyCache();
    void insertKeyToCache(string& hash, string& key);
    bool existsKeyinCache(string& hash);
    string getKeyFromCache(string& hash);
};

#endif //GENERALDEDUPSYSTEM_CACHE_HPP
