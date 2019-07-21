#include <cache.hpp>

using namespace std;
using namespace boost::compute::detail;

keyCache::keyCache()
{
    this->lruKeyCache_ = new lru_cache<string, string>(config.getKeyCacheSize());
}

keyCache::~keyCache()
{
    delete lruKeyCache_;
}

void keyCache::insertKeyToCache(string& hash, string& key)
{
    std::lock_guard<std::mutex> locker(this->mutexKeyCache_);
    this->lruKeyCache_->insert(hash, key);
}

bool keyCache::existsKeyinCache(string& hash)
{
    bool flag = false;
    std::lock_guard<std::mutex> locker(this->mutexKeyCache_);
    flag = this->lruKeyCache_->contains(hash);
    return flag;
}

string keyCache::getKeyFromCache(string& hash)
{
    string ans = "";
    if (!existsKeyinCache(hash)) {
        return ans;
    } else {
        std::lock_guard<std::mutex> locker(this->mutexKeyCache_);
        ans = this->lruKeyCache_->get(hash).get();
        return ans;
    }
}
