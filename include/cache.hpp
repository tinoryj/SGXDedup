#ifndef GENERALDEDUPSYSTEM_CACHE_HPP
#define GENERALDEDUPSYSTEM_CACHE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include <bits/stdc++.h>
#include <boost/compute/detail/lru_cache.hpp>
#include <boost/thread.hpp>

extern Configure config;

using namespace boost::compute::detail;
using namespace std;

class KeyCache {
private:
    lru_cache<string, string>* lruKeyCache_;
    std::mutex keyCacheMutex_;

public:
    KeyCache();
    ~KeyCache();
    void insertKeyToCache(string& hash, string& key);
    bool existsKeyinCache(string& hash);
    string getKeyFromCache(string& hash);
};

typedef struct {
    int cnt_;
    bool avaiable_;
    string chunkLogicData_;
} ChunkCache_t;

class ChunkCache {
private:
    map<string, ChunkCache_t*> memBuffer_;
    CryptoPrimitive* cryptoObj_;
    std::mutex chunkCacheMutex_;

public:
    ChunkCache();
    ~ChunkCache() {}
    void refer(string& chunkHash);
    void derefer(string& chunkHash);
    void setChunk(vector<string>& fp, vector<string>& chunks);
    bool readChunk(string& chunkHash, string& chunkLogicData);
};

#endif //GENERALDEDUPSYSTEM_CACHE_HPP
