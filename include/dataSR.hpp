#ifndef GENERALDEDUPSYSTEM_DATASR_HPP
#define GENERALDEDUPSYSTEM_DATASR_HPP

#include "../src/pow/include/powServer.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "dedupCore.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "socket.hpp"
#include "storageCore.hpp"
#include "sys/epoll.h"
#include <bits/stdc++.h>

using namespace std;

extern Configure config;

class DataSR {
private:
    StorageCore* storageObj_;
    DedupCore* dedupCoreObj_;
    powServer* powServerObj_;

public:
    DataSR(StorageCore* storageObj, DedupCore* dedupCoreObj, powServer* powServerObj);
    ~DataSR(){};
    void run(Socket socket);
};

#endif //GENERALDEDUPSYSTEM_DATASR_HPP
