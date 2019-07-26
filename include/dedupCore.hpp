#ifndef GENERALDEDUPSYSTEM_DEDUPCORE_HPP
#define GENERALDEDUPSYSTEM_DEDUPCORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>
#include <boost/thread.hpp>

using namespace std;

class signedHash {
public:
    signedHashList_t signedHashList_;
    messageQueue<signedHashList_t> outPutMQ_;
    bool checkDone();
    void timeout();
};

class Timer {
private:
    struct cmp {
        bool operator()(signedHash* x, signedHash* y)
        {
            return x->signedHashList_.startTime.time_since_epoch().count() > y->signedHashList_.startTime.time_since_epoch().count();
        }
    };
    priority_queue<signedHash*, vector<signedHash*>, cmp> jobQueue_;
    std::mutex timerMutex_;

public:
    void registerHashList(signedHash* job);
    void run();
    void startTimer();
};

class dedupCore {
private:
    messageQueue<Chunk_t> netSendMQ_;
    messageQueue<Chunk_t> powMQ_;
    CryptoPrimitive* cryptoObj_;
    Timer timer_;
    bool dedupStage1(powSignedHash in, RequiredChunk& out);
    bool dedupStage2(ChunkList_t in);

public:
    void run();
    bool dataDedup();
    dedupCore();
    ~dedupCore();
};

#endif //GENERALDEDUPSYSTEM_DEDUPCORE_HPP
