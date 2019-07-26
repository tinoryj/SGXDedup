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

chunkCache_t::chunkCache_t()
{
    this->cnt_ = 1;
    this->avaiable_ = false;
    this->chunkLogicData_.clear();
}

void chunkCache_t::refer()
{
    {
        std::lock_guard<std::mutex> locker(this->cntMutex_);
        this->cnt_++;
    }
}

void chunkCache_t::derefer()
{
    {
        std::lock_guard<std::mutex> locker(this->cntMutex_);
        this->cnt_--;
    }
}

int chunkCache_t::readCnt()
{
    int ans;
    {
        std::lock_guard<std::mutex> locker(this->cntMutex_);
        ans = this->cnt_;
    }
    return ans;
}

void chunkCache_t::setChunk(string& chunkLogicData)
{
    {
        this->chunkLogicData_ = chunkLogicData;
        std::lock_guard<std::mutex> locker(this->avaiMutex_);
        this->avaiable_ = true;
    }
}

bool chunkCache_t::readChunk(string& chunkLogicData)
{
    bool status;
    {
        std::lock_guard<std::mutex> locker(this->avaiMutex_);
        status = this->avaiable_;
    }

    if (status) {
        chunkLogicData = this->chunkLogicData_;
    }
    return status;
}

chunkCache::chunkCache()
{
    this->cryptoObj_ = new CryptoPrimitive();
}

void chunkCache::refer(string& chunkHash)
{
    map<string, chunkCache_t*>::iterator it;
    {
        std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
        it = memBuffer_.find(chunkHash);
        if (it == memBuffer_.end()) {
            chunkCache_t* tmp = new chunkCache_t();
            memBuffer_.insert(make_pair(chunkHash, tmp));
        }
    }
}

void chunkCache::derefer(string& chunkHash)
{
    map<string, chunkCache_t*>::iterator it;
    {
        std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
        it = memBuffer_.find(chunkHash);
        if (it == memBuffer_.end()) {
            return;
        }
        it->second->derefer();
        if (it->second->readCnt() == 0) {
            memBuffer_.erase(it);
        }
    }
}

void chunkCache::setChunk(vector<string>& fp, vector<string>& chunks)
{
    int size = fp.size();
    for (int i = 0; i < size; i++) {
        map<string, chunkCache_t*>::iterator it1;
        {
            std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
            it1 = memBuffer_.find(fp[i]);
            if (it1 != memBuffer_.end()) {
                it1->second->setChunk(chunks[i]);
            }
        }
    }
}

bool chunkCache::readChunk(string& chunkHash, string& chunkLogicData)
{
    map<string, chunkCache_t*>::iterator it;
    bool status;
    {
        std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
        it = memBuffer_.find(chunkHash);
        if (it == memBuffer_.end()) {
            return false;
        }
        status = it->second->readChunk(chunkLogicData);
    }
    return status;
}