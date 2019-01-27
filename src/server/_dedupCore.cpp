#include "_dedupCore.hpp"

chunkCache cache;
extern database fp2ChunkDB;
extern Configure config;

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
    this->_crypto=new CryptoPrimitive(SHA256_TYPE);
}

void chunkCache::refer(string &chunkHash) {
    map<string,chunkCache_t*>::iterator it;
    {
        boost::unique_lock<boost::shared_mutex> t(this->_mtx);
        it=_memBuffer.find(chunkHash);
        if(it==map::end()){
            chunkCache_t tmp;
            _memBuffer.insert(make_pair(chunkHash,tmp));
        }
    }
}

void chunkCache::derefer(string &chunkHash) {
    map<string,chunkCache_t*>::iterator it;
    {
        boost::unique_lock<boost::shared_mutex> t(this->_mtx);
        it=_memBuffer.find(chunkHash);
        if(it==map::end()){
            return;
        }
        it->second->derefer();
        if(it->second->readCnt()==0){
            _memBuffer.erase(it);
        }
    }
}

void chunkCache::setChunk(vector<string> &fp,vector<string> &chunks) {
    int size=fp.size();
    for(int i=0;i<size;i++){
        map<string,chunkCache_t*>::iterator it1;
        {
            boost::unique_lock<boost::shared_mutex> t(this->_mtx);
            it1 = _memBuffer.find(fp[i]);
            if (it1!= map::end()) {
                it1->second->setChunk(fp[i]);
            }
        }
    }
}

bool chunkCache::readChunk(string &chunkHash, string &chunkLogicData) {
    map<string,chunkCache_t*>::iterator it;
    bool status;
    {
        boost::shared_lock<boost::shared_mutex> t(this->_mtx);
        it=_memBuffer.find(chunkHash);
        if(it==map::end()){
            return false;
        }
        status=it->second->readChunk(chunkLogicData);
    }
    return status;
}

void signedHash::setMQ(_messageQueue mq) {
    this->_outputMQ=mq;
}

bool signedHash::checkDone() {
    string chunkLogicData;
    bool success;
    for(auto it:this->_hashList){
        success=cache.readChunk(it,chunkLogicData);
        if(!success){
            return false;
        }
        _chunks.push_back(chunkLogicData);
    }

    //完善协议
    int size=_hashList.size();
    for(int i=0;i<size;i++){
        _outputMQ.push(_hashList[i]+_chunks[i]);
    }
    return true;
}

void signedHash::timeout() {
    int size=_chunks.size();
    for(int i=0;i<size;i++){
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
    std::chrono::time_point now;
    std::chrono::duration<int,std::milli> dtn;
    bool emptyFlag;
    while (1) {
        {
            boost::unique_lock<boost::shared_mutex> t(this->_mtx);
            emptyFlag = _jobQueue.empty();
            if (!emptyFlag)
                nowJob = _jobQueue.top();
            _jobQueue.pop();
        }
        if(emptyFlag){
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }
        now = std::chrono::high_resolution_clock::now();
        dtn=now-nowJob->_stopTime;
        if(dtn.count()-nowJob->_outDataTime>0){
            this_thread::sleep_for(chrono::milliseconds(dtn.count()-nowJob->_outDataTime));
        }
        if(!nowJob->checkDone()){
            nowJob->timeout();
        }
        delete nowJob;
    }

}

void Timer::startTimer() {
    boost::thread th(boost::bind(Timer::run,this));
    th.detach();
}

_DedupCore::_DedupCore() {
    _crypto=new CryptoPrimitive(SHA256_TYPE);
    _inputMQ.createQueue("DataSR to dupCore",READ_MESSAGE);
    _outputMQ.createQueue("dedupCore to storageCore",WRITE_MESSAGE);
    _netSendMQ.createQueue("DataSRSEND",WRITE_MESSAGE);
    _timer.startTimer();
}

_DedupCore::~_DedupCore() {
    if(_crypto!= nullptr){
        delete _crypto;
    }
}

bool _DedupCore::dedupStage1(powSignedHash in, RequiredChunk out) {
    out.clear();
    bool status = true;
    //verifysign

    signedHash *sig=new signedHash();
    sig->setMQ(_outputMQ);


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
    sig->_stopTime=std::chrono::high_resolution_clock::now();
    _timer.registerHashList(sig);

    return status;
}

bool _DedupCore::dedupStage2(chunkList in) {
    string hash;
    int size=in._chunks.size(),i;
    for(i=0;i<size;i++){
        _crypto->generaHash(in._chunks[i],hash);
        if(hash!=in._FP[i]){
            break;
        }
    }

    if(i==size){
        cache.setChunk(in._FP,in._chunks);
        return true;
    }
    return false;
}

void _DedupCore::run() {
    epollMessageStruct msg1;
    networkStruct msg2;
    powSignedHash msg3;
    RequiredChunk msg4;
    chunkList msg5;

    while(1){
        _inputMQ.pop(msg1);
        deserialize(msg1._data,msg2);
        switch(msg2._type){
            case SGX_SIGNED_HASH:{
                deserialize(msg2._data,msg3);
                if(this->dedupStage1(msg3,msg4)){
                    msg2._type=OK;
                }
                else{
                    msg2._type=ERROR_RESEND;
                }
                serialize(msg4,msg2._data);
                serialize(msg2,msg1._data);
                _netSendMQ.push(msg1);
                break;
            }
            case CLIENT_UPLOAD_CHUNK:{
                deserialize(msg2._data,msg5);
                if(this->dedupStage2(msg5)){
                    msg2._type=OK;
                }
                else{
                    msg2._type=ERROR_RESEND;
                }
                msg2._data.clear();
                serialize()
            }
        }
    }
}