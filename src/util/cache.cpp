#include "cache.hpp"
#include <boost/thread/mutex.hpp>

Cache::Cache()
{
    cacheSize_ = 128;
    this->Cache_ = new lru11::Cache<string, uint32_t>(cacheSize_, 0);
    containerPool_ = (uint8_t**)malloc(cacheSize_ * sizeof(uint8_t*));
    for (size_t i = 0; i < cacheSize_; i++) {
        containerPool_[i] = (uint8_t*)malloc(config.getMaxContainerSize() * sizeof(uint8_t));
    }
    currentIndex_ = 0;
#if SYSTEM_LOG_FLAG == 1
    cerr << "Container Cache Size = " << this->cacheSize_ << endl;
#endif
}

void Cache::insertToCache(string& name, uint8_t* data, uint32_t length)
{
    {
        boost::unique_lock<boost::shared_mutex> t(this->mtx);
        if (Cache_->size() + 1 > cacheSize_) {
            // evict a item
            uint32_t replaceIndex = Cache_->pruneValue();
            memset(containerPool_[replaceIndex], 0, config.getMaxContainerSize());
            memcpy(containerPool_[replaceIndex], data, length);
            Cache_->insert(name, replaceIndex);
        } else {
            // directly using current index
            memset(containerPool_[currentIndex_], 0, config.getMaxContainerSize());
            memcpy(containerPool_[currentIndex_], data, length);
            Cache_->insert(name, currentIndex_);
            currentIndex_++;
        }
    }
}

bool Cache::existsInCache(string& name)
{
    bool flag = false;
    {
        boost::shared_lock<boost::shared_mutex> t(this->mtx);
        flag = this->Cache_->contains(name);
    }
    return flag;
}

uint8_t* Cache::getFromCache(string& name)
{
    {
        boost::shared_lock<boost::shared_mutex> t(this->mtx);
        uint32_t index = this->Cache_->get(name);
        return containerPool_[index];
    }
}

Cache::~Cache()
{
    for (size_t i = 0; i < cacheSize_; i++) {
        free(containerPool_[i]);
    }
    free(containerPool_);
    delete Cache_;
}