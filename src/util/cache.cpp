#include <cache.hpp>

using namespace std;
using namespace boost::compute::detail;

KeyCache::KeyCache()
{
    this->lruKeyCache_ = new lru_cache<string, string>(config.getKeyCacheSize());
}

KeyCache::~KeyCache()
{
    delete lruKeyCache_;
}

void KeyCache::insertKeyToCache(string& hash, string& key)
{
    std::lock_guard<std::mutex> locker(this->keyCacheMutex_);
    this->lruKeyCache_->insert(hash, key);
}

bool KeyCache::existsKeyinCache(string& hash)
{
    bool flag = false;
    std::lock_guard<std::mutex> locker(this->keyCacheMutex_);
    flag = this->lruKeyCache_->contains(hash);
    return flag;
}

string KeyCache::getKeyFromCache(string& hash)
{
    string ans = "";
    if (!existsKeyinCache(hash)) {
        return ans;
    } else {
        std::lock_guard<std::mutex> locker(this->keyCacheMutex_);
        ans = this->lruKeyCache_->get(hash).get();
        return ans;
    }
}

ChunkCache::ChunkCache()
{
    this->cryptoObj_ = new CryptoPrimitive();
}

void ChunkCache::refer(string& chunkHash)
{
    map<string, ChunkCache_t*>::iterator it;
    {
        std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
        it = memBuffer_.find(chunkHash);
        if (it == memBuffer_.end()) {
            ChunkCache_t* tmp = new ChunkCache_t();
            memBuffer_.insert(make_pair(chunkHash, tmp));
        }
    }
}

void ChunkCache::derefer(string& chunkHash)
{
    map<string, ChunkCache_t*>::iterator it;
    {
        std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
        it = memBuffer_.find(chunkHash);
        if (it == memBuffer_.end()) {
            return;
        }
        memBuffer_.erase(it);
    }
}

void ChunkCache::setChunk(vector<string>& fp, vector<string>& chunks)
{
    int size = fp.size();
    for (int i = 0; i < size; i++) {
        map<string, ChunkCache_t*>::iterator it1;
        {
            std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
            it1 = memBuffer_.find(fp[i]);
            if (it1 != memBuffer_.end()) {
                it1->second->chunkLogicData_.resize(chunks[i].length());
                memcpy(&it1->second->chunkLogicData_[0], &chunks[i][0], chunks[i].length());
            }
        }
    }
}

bool ChunkCache::readChunk(string& chunkHash, string& chunkLogicData)
{
    map<string, ChunkCache_t*>::iterator it;
    bool status = true;
    {
        std::lock_guard<std::mutex> locker(this->chunkCacheMutex_);
        it = memBuffer_.find(chunkHash);
        if (it == memBuffer_.end()) {
            return false;
        }
        chunkLogicData.resize(it->second->chunkLogicData_.length());
        memcpy(&chunkLogicData[0], &it->second->chunkLogicData_[0], it->second->chunkLogicData_.length());
    }
    return status;
}
