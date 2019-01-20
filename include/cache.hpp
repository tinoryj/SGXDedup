//
// Created by a on 11/19/18.
//

#ifndef GENERALDEDUPSYSTEM_CACHE_HPP
#define GENERALDEDUPSYSTEM_CACHE_HPP

#include "boost/compute/detail/lru_cache.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include <string>

extern Configure config;

using namespace boost::compute::detail;
using namespace std;

namespace util{
    class keyCache{
    private:
        lru_cache<string,string>* _kCache;
        boost::shared_mutex mtx;
    public:
        keyCache();
        void insertKeyToCache(string& hash,string& key);
        bool existsKeyinCache(string& hash);
        string getKeyFromCache(string& hash);
    };
}

#endif //GENERALDEDUPSYSTEM_CACHE_HPP
