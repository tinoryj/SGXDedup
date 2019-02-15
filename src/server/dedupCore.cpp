//
// Created by a on 2/3/19.
//

#include "dedupCore.hpp"


chunkCache cache;

extern database fp2ChunkDB;
extern Configure config;

dedupCore::dedupCore() {
    _netSendMQ.createQueue(DATASR_IN_MQ,WRITE_MESSAGE);
    _powMQ.createQueue(POWSERVER_TO_DEDUPCORE_MQ,READ_MESSAGE);
    _crypto=new CryptoPrimitive();
    _timer.startTimer();
}

/*
 * dedupCore thread handle
 * process two type message:
 *      sgx_signed_hash     : for dedup stage one, regist to timer
 *      client_upload_chunk : for dedup stage two, save chunk to cache
 * */
void dedupCore::run() {
    epoll_message msg1;
    networkStruct msg2;
    powSignedHash msg3;
    RequiredChunk msg4;
    chunkList msg5;

    while (1) {
        if(this->extractMQ(msg1)) {
            deserialize(msg1._data, msg2);
            switch (msg2._type) {
                case CLIENT_UPLOAD_CHUNK: {
                    deserialize(msg2._data, msg5);
                    if (this->dedupStage2(msg5)) {
                        msg2._type = SUCCESS;
                    } else {
                        msg2._type = ERROR_RESEND;
                    }
                    msg2._data.clear();
                    serialize(msg2, msg1._data);
                    _netSendMQ.push(msg1);
                }
            }
        }

        if(_powMQ.pop(msg1)){
            deserialize(msg1._data, msg2);
            switch (msg2._type) {
                case SGX_SIGNED_HASH: {
                    deserialize(msg2._data, msg3);
                    if (this->dedupStage1(msg3, msg4)) {
                        msg2._type = SUCCESS;
                    } else {
                        msg2._type = ERROR_RESEND;
                    }
                    serialize(msg4, msg2._data);
                    serialize(msg2, msg1._data);
                    _netSendMQ.push(msg1);
                    break;
                }
            }
        }
    }
}

bool dedupCore::dedupStage1(powSignedHash in, RequiredChunk out) {
    out.clear();
    bool status = true;
    //verifysign

    signedHash *sig = new signedHash();
    sig->setMQ(this->getOutputMQ());


    if (status) {
        string tmpdata;
        int size = in.hash.size();
        for (int i = 0; i < size; i++) {
            if (fp2ChunkDB.query(in.hash[i], tmpdata)) {
                continue;
            }
            out.push_back(i);
            sig->_hashList.push_back(in.hash[i]);
        }
    }

    /* register unique chunk list to timer
     * when time out,timer will check all chunk received or nor
     * if yes, then push all chunk to storage
     * or not, then delete(de-refer) all chunk in cache
     */
    sig->_stopTime = std::chrono::high_resolution_clock::now();
    _timer.registerHashList(sig);

    return status;
}

bool dedupCore::dedupStage2(chunkList in) {
    string hash;
    int size = in._chunks.size(), i;
    for (i = 0; i < size; i++) {
        //check chunk hash correct or not
        _crypto->sha256_digest(in._chunks[i],hash);
        if (hash != in._FP[i]) {
            break;
        }
    }

    /* if i != size, then client are not honest,it modified some chunk's hash
     * then do not store store chunk come form client
    */
    if (i == size) {
        cache.setChunk(in._FP, in._chunks);
        return true;
    }
    return false;
}


chunkCache_t::chunkCache_t() {
    this->_cnt=1;
    this->_avaiable;
    this->_chunkLogicData.clear();

}

void chunkCache_t::refer() {
    {
        boost::unique_lock<boost::shared_mutex> t(this->_cntMtx);
        this->_cnt++;
    }
}

void chunkCache_t::derefer() {
    {
        boost::unique_lock<boost::shared_mutex> t(this->_cntMtx);
        this->_cnt--;
    }
}

int chunkCache_t::readCnt() {
    int ans;
    {
        boost::shared_lock<boost::shared_mutex> t(this->_cntMtx);
        ans=this->_cnt;
    }
    return ans;
}

void chunkCache_t::setChunk(string &chunkLogicData) {
    {
        this->_chunkLogicData=chunkLogicData;
        boost::unique_lock<boost::shared_mutex> t(this->_avaiMtx);
        this->_avaiable= true;
    }
}

bool chunkCache_t::readChunk(string &chunkLogicData) {
    bool status;
    {
        boost::shared_lock<boost::shared_mutex> t(this->_avaiMtx);
        status=this->_avaiable;
    }
    if(status){
        chunkLogicData=this->_chunkLogicData;
    }
    return status;
}

chunkCache::chunkCache() {
    this->_crypto=new CryptoPrimitive();
}

void chunkCache::refer(string &chunkHash) {
    map<string, chunkCache_t *>::iterator it;
    {
        boost::unique_lock<boost::shared_mutex> t(this->_mtx);
        it = _memBuffer.find(chunkHash);
        if (it == _memBuffer.end()) {
            chunkCache_t *tmp = new chunkCache_t();
            _memBuffer.insert(make_pair(chunkHash, tmp));
        }
    }
}

void chunkCache::derefer(string &chunkHash) {
    map<string, chunkCache_t *>::iterator it;
    {
        boost::unique_lock<boost::shared_mutex> t(this->_mtx);
        it = _memBuffer.find(chunkHash);
        if (it == _memBuffer.end()) {
            return;
        }
        it->second->derefer();
        if (it->second->readCnt() == 0) {
            _memBuffer.erase(it);
        }
    }
}

void chunkCache::setChunk(vector<string> &fp,vector<string> &chunks) {
    int size = fp.size();
    for (int i = 0; i < size; i++) {
        map<string, chunkCache_t *>::iterator it1;
        {
            boost::unique_lock<boost::shared_mutex> t(this->_mtx);
            it1 = _memBuffer.find(fp[i]);
            if (it1 != _memBuffer.end()) {
                it1->second->setChunk(fp[i]);
            }
        }
    }
}

bool chunkCache::readChunk(string &chunkHash, string &chunkLogicData) {
    map<string, chunkCache_t *>::iterator it;
    bool status;
    {
        boost::shared_lock<boost::shared_mutex> t(this->_mtx);
        it = _memBuffer.find(chunkHash);
        if (it == _memBuffer.end()) {
            return false;
        }
        status = it->second->readChunk(chunkLogicData);
    }
    return status;
}

void signedHash::setMQ(_messageQueue mq) {
    this->_outputMQ = mq;
}

bool signedHash::checkDone() {
    string chunkLogicData;
    bool success;
    for (auto it:this->_hashList) {
        success = cache.readChunk(it, chunkLogicData);
        if (!success) {
            return false;
        }
        _chunks.push_back(chunkLogicData);
    }

    //protocol for dedupcore and storagecore
    chunkList chunks;
    int size = _hashList.size();
    for (int i = 0; i < size; i++) {
        chunks._FP.push_back(_hashList[i]);
        chunks._chunks.push_back(_chunks[i]);
    }
    _outputMQ.push(chunks);

    return true;
}

void signedHash::timeout() {
    int size = _chunks.size();
    for (int i = 0; i < size; i++) {
        cache.derefer(_hashList[i]);
    }
//  _hashList.clear();
//  _chunks.clear();
}

void Timer::registerHashList(signedHash *job) {
    {
        boost::unique_lock<boost::shared_mutex> t(this->_mtx);
        _jobQueue.push(job);
    }
}

void Timer::run() {
    signedHash *nowJob;
    std::chrono::system_clock::time_point now;
    std::chrono::system_clock::duration dtn;
    bool emptyFlag;
    while (1) {
        {
            boost::unique_lock<boost::shared_mutex> t(this->_mtx);
            emptyFlag = _jobQueue.empty();
            if (!emptyFlag) {
                nowJob = _jobQueue.top();
                _jobQueue.pop();
            }
        }
        if (emptyFlag) {
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }
        now = std::chrono::high_resolution_clock::now();
        dtn = now - nowJob->_stopTime;
        if (dtn.count() - nowJob->_outDataTime > 0) {
            this_thread::sleep_for(chrono::milliseconds(dtn.count() - nowJob->_outDataTime));
        }
        if (!nowJob->checkDone()) {
            nowJob->timeout();
        }
        delete nowJob;
    }

}

void Timer::startTimer() {
    boost::thread th(boost::bind(&Timer::run, this));
    th.detach();
}

bool dedupCore::dataDedup() {}