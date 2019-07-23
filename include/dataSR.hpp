#ifndef GENERALDEDUPSYSTEM_DATASR_HPP
#define GENERALDEDUPSYSTEM_DATASR_HPP

#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include "socket.hpp"
#include "sys/epoll.h"
#include <bits/stdc++.h>

using namespace std;

class dataSR {
private:
    messageQueue<EpollMessage_t> MQ2DataSR_CallBack_;
    messageQueue<EpollMessage_t> MQ2DedupCore_;
    messageQueue<EpollMessage_t> MQ2StorageCore_;
    messageQueue<EpollMessage_t> MQ2RAServer_;

    Socket _socket;
    map<int, EpollMessage_t*> _epollSession;
    std::mutex epollSessionMutex_;

public:
    dataSR();
    ~dataSR();
    void run();
    bool receiveData();
    bool sendData();
    bool workloadProgress();
    bool insertMQ2DedupCore(EpollMessage_t& newMessage);
    bool insertMQ2StorageCore(EpollMessage_t& newMessage);
    bool insertMQ2RAServer(EpollMessage_t& newMessage);
    bool extractMQ2DataSR_CallBack(EpollMessage_t& newMessage);
};

#endif //GENERALDEDUPSYSTEM_DATASR_HPP
