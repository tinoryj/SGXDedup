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

class chunkCache_t {
private:
    int cnt_;
    bool avaiable_;
    string chunkLogicData_;
    std::mutex cntMutex_, avaiMutex_;

public:
    chunkCache_t();
    void refer();
    void derefer();
    int readCnt();
    void setChunk(string& chunkLogicData);
    bool readChunk(string& chunkLogicData);
};

class chunkCache {
private:
    map<string, chunkCache_t*> memBuffer_;
    CryptoPrimitive* cryptoObj_;
    std::mutex chunkCacheMutex_;

public:
    chunkCache();
    ~chunkCache() {}
    void refer(string& chunkHash);
    void derefer(string& chunkHash);
    void setChunk(vector<string>& fp, vector<string>& chunks);
    bool readChunk(string& chunkHash, string& chunkLogicData);
};

#endif //GENERALDEDUPSYSTEM_CACHE_HPP
