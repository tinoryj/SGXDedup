//
// Created by a on 11/19/18.
//

#ifndef GENERALDEDUPSYSTEM_CACHE_HPP
#define GENERALDEDUPSYSTEM_CACHE_HPP

#include <boost/compute/detail/lru_cache.hpp>
#include <string>

using namespace boost::compute::detail;
using namespace std;

namespace util{
    namespace keyCache{
        void insertKeyToCache(lru_cache<string,string>& kCache,string hash,string key);
        bool existsKeyinCache(lru_cache<string,string>& kCache,string hash);
        string getKeyFromCache(lru_cache<string,string>& kcache,string hash);
    }
}

#endif //GENERALDEDUPSYSTEM_CACHE_HPP
