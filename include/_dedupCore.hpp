#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "protocol.hpp"
#include "message.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "CryptoPrimitive.hpp"
#include "database.hpp"
#include "time.h"
#include "chrono"
#include "thread"
#include <chrono>
#include <algorithm>
#include <queue>

#include "tmp.hpp"

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

    std::chrono::time_point _stopTime;

    void setMQ(_messageQueue mq);
    bool checkDone();
    void timeout();
};

class Timer{
private:
    struct cmp{
        bool operator()(signedHash* x,signedHash* y){
            std::chrono::duration<int,std::milli> tx,ty;
            tx=x->_stopTime;
            ty=x->_stopTime;
            return tx.count()>ty.count();
        }
    };
    priority_queue<signedHash*,vector<signedHash*>,cmp> _jobQueue;
    boost::shared_mutex _mtx;
public:
    void registerHashList(signedHash *job);
    void run();
    void startTimer();
};


class _DedupCore {
    private:
        _messageQueue _inputMQ;
        _messageQueue _outputMQ;
        _messageQueue _netSendMQ;
        CryptoPrimitive* _crypto;

        Timer _timer;

       //std::vector<leveldb::DB> _dbSet;
       // any additional info
        bool dedupStage1(powSignedHash in, RequiredChunk out);
        bool dedupStage2(chunkList in);
public:
        _DedupCore();
        ~_DedupCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool dataDedup() = 0;
        //MessageQueue getInputMQ();
        //MessageQueue getOutputMQ();
        //leveldb::DB getDBObject(int dbPosition);
        // any additional functions
        void run();
};
