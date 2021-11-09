#ifndef SGXDEDUP_DATASR_HPP
#define SGXDEDUP_DATASR_HPP

#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "dedupCore.hpp"
#include "kmServer.hpp"
#include "messageQueue.hpp"
#include "powServer.hpp"
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
    kmServer* kmServerObj_;
    uint32_t restoreChunkBatchNumber_;
    u_char keyExchangeKey_[KEY_SERVER_SESSION_KEY_SIZE];
    bool keyExchangeKeySetFlag_;
    ssl* powSecurityChannel_;
    ssl* dataSecurityChannel_;
    uint64_t keyRegressionCurrentTimes_;
#if MULTI_CLIENT_UPLOAD_TEST == 1
    std::mutex mutexSessions_;
    std::mutex mutexCrypto_;
    std::mutex mutexRestore_;
#endif
public:
    enclaveSession* keyServerSession_;
    DataSR(StorageCore* storageObj, DedupCore* dedupCoreObj, powServer* powServerObj, kmServer* kmServerObj, ssl* powSecurityChannelTemp, ssl* dataSecurityChannelTemp);
    ~DataSR() {};
    void runData(SSL* sslConnection);
    void runPow(SSL* sslConnection);
    void runKeyServerRemoteAttestation();
    void runKeyServerSessionKeyUpdate();
};

#endif //SGXDEDUP_DATASR_HPP
