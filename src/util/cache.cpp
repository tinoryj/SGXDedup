//
// Created by a on 11/19/18.
//

#include <cache.hpp>
using namespace std;
using namespace boost::compute::detail;

void util::keyCache::insertKeyToCache(lru_cache<string, string> &kCache, string hash, string key) {
    kCache.insert(hash,key);
}

bool util::keyCache::existsKeyinCache(lru_cache<string, string> &kCache, string hash) {
    return kCache.contains(hash);
}

string util::keyCache::getKeyFromCache(lru_cache<string, string> &kcache, string hash) {
    if(!existsKeyinCache(kcache,hash))return "";
    return kcache.get(hash).get();
}
