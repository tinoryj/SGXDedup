#ifndef GENERALDEDUPSYSTEM_DATASR_HPP
#define GENERALDEDUPSYSTEM_DATASR_HPP

#include "../src/pow/include/powServer.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "dedupCore.hpp"
#include "kmServer.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "socket.hpp"
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
    u_char keyExchangeKey_[16];
    bool keyExchangeKeySetFlag;

public:
    DataSR(StorageCore* storageObj, DedupCore* dedupCoreObj, powServer* powServerObj);
    ~DataSR(){};
    void run(Socket socket);
    void runPow(Socket socket);
    void runKeyServerRA();
};

#endif //GENERALDEDUPSYSTEM_DATASR_HPP
