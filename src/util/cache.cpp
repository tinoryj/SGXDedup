//
// Created by a on 11/19/18.
//

#include <cache.hpp>
#include <boost/thread/mutex.hpp>

using namespace std;
using namespace boost::compute::detail;


namespace util {

    keyCache::keyCache(){
        this->_kCache=new lru_cache<string,string>(config.getKeyCacheSize());
    }

    void keyCache::insertKeyToCache(string hash, string key) {
        {
            boost::unique_lock<boost::shared_mutex> t(this->mtx);
            this->_kCache->insert(hash, key);
        }
    }

    bool util::keyCache::existsKeyinCache(string hash) {
        bool flag=false;
        {
            boost::shared_lock<boost::shared_mutex> t(this->mtx);
            flag=this->_kCache->contains(hash);
        }
        return flag;
    }

    string util::keyCache::getKeyFromCache(string hash) {
        string ans="";
        if (!existsKeyinCache(hash))return ans;
        {
            boost::shared_lock<boost::shared_mutex> t(this->mtx);
            ans=this->_kCache->get(hash).get();
        }
        return ans;
    }
}