#ifndef GENERALDEDUPSYSTEM_DATASR_HPP
#define GENERALDEDUPSYSTEM_DATASR_HPP

#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "socket.hpp"
#include "sys/epoll.h"
#include "timer.hpp"
#include <bits/stdc++.h>

using namespace std;

class DataSR {
private:
    messageQueue<EpollMessage_t> MQ2DataSR_CallBack_;
    messageQueue<EpollMessage_t> MQ2DedupCore_;
    messageQueue<EpollMessage_t> MQ2StorageCore_;
    messageQueue<EpollMessage_t> MQ2RAServer_;
    Socket socket_;
    Configure* config_;
    map<int, EpollMessage_t*> epollSession_;
    std::mutex epollSessionMutex_;

public:
    Timer* timerObj_;
    DataSR();
    ~DataSR();
    void run();
    bool receiveData();
    bool sendData();
    bool workloadProgress();
    bool extractMQ();
    bool insertMQ(int queueSwitch, EpollMessage_t& msg);
    bool insertMQ2DedupCore(EpollMessage_t& newMessage);
    bool insertMQ2StorageCore(EpollMessage_t& newMessage);
    bool insertMQ2RAServer(EpollMessage_t& newMessage);
    bool insertMQ2DataSR_CallBack(EpollMessage_t& newMessage);
    bool extractMQ2DedupCore(EpollMessage_t& newMessage);
    bool extractMQ2StorageCore(EpollMessage_t& newMessage);
    bool extractMQ2RAServer(EpollMessage_t& newMessage);
    bool extractMQ2DataSR_CallBack(EpollMessage_t& newMessage);
    bool extractTimerMQToStorageCore(StorageCoreData_t& newData);
    bool insertTimerMQToStorageCore(StorageCoreData_t& newData);
};

#endif //GENERALDEDUPSYSTEM_DATASR_HPP
