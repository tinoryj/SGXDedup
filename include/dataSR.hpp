#ifndef SGXDEDUP_DATASR_HPP
#define SGXDEDUP_DATASR_HPP

#include "../src/pow/include/powServer.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "dedupCore.hpp"
#include "kmServer.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "ssl.hpp"
#include "storageCore.hpp"
#include <bits/stdc++.h>

using namespace std;

extern Configure config;

class DataSR {
private:
    StorageCore* storageObj_;
    DedupCore* dedupCoreObj_;
    powServer* powServerObj_;
    uint32_t restoreChunkBatchSize;
    u_char keyExchangeKey_[KEY_SERVER_SESSION_KEY_SIZE];
    bool keyExchangeKeySetFlag;
    ssl* powSecurityChannel_;
    ssl* dataSecurityChannel_;
    uint64_t keyRegressionCurrentTimes_;

public:
    DataSR(StorageCore* storageObj, DedupCore* dedupCoreObj, powServer* powServerObj, ssl* powSecurityChannelTemp, ssl* dataSecurityChannelTemp);
    ~DataSR() {};
    void run(SSL* sslConnection);
    void runPow(SSL* sslConnection);
    void runKeyServerRA();
};

#endif //SGXDEDUP_DATASR_HPP
