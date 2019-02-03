//
// Created by a on 2/3/19.
//

#ifndef GENERALDEDUPSYSTEM_DEDUPCORE_HPP
#define GENERALDEDUPSYSTEM_DEDUPCORE_HPP

#include "_dedupCore.hpp"


//compile -stdc++



class chunkCache_t{
private:
    int _cnt;
    bool _avaiable;
    string _chunkLogicData;
    boost::shared_mutex _cntMtx,_avaiMtx;
public:
    chunkCache_t();
    void refer();
    void derefer();
    int readCnt();
    void setChunk(string &chunkLogicData);
    bool readChunk(string &chunkLogicData);
};

class chunkCache{
private:
    map<string,chunkCache_t*>_memBuffer;
    CryptoPrimitive *_crypto;
    boost::shared_mutex _mtx;
public:
    chunkCache();
    void refer(string &chunkHash);
    void derefer(string &chunkHash);
    void setChunk(vector<string> &fp,vector<string> &chunks);
    bool readChunk(string &chunkHash,string &chunkLogicData);
};

class signedHash{
public:
    vector<string>_hashList;
    vector<string>_chunks;
    _messageQueue _outputMQ;
    int _outDataTime;

    std::chrono::system_clock::time_point _stopTime;

    void setMQ(_messageQueue mq);
    bool checkDone();
    void timeout();
};

class Timer{
private:
    struct cmp{
        bool operator()(signedHash* x,signedHash* y){
            return x->_stopTime.time_since_epoch().count()>y->_stopTime.time_since_epoch().count();
        }
    };
    priority_queue<signedHash*,vector<signedHash*>,cmp> _jobQueue;
    boost::shared_mutex _mtx;
public:
    void registerHashList(signedHash *job);
    void run();
    void startTimer();
};


class dedupCore:public _DedupCore{
private:
    _messageQueue _netSendMQ;
    CryptoPrimitive* _crypto;
    Timer _timer;
    bool dedupStage1(powSignedHash in, RequiredChunk out);
    bool dedupStage2(chunkList in);
public:
    void run();
    bool dataDedup();
    dedupCore();
    ~dedupCore();
};


#endif //GENERALDEDUPSYSTEM_DEDUPCORE_HPP
