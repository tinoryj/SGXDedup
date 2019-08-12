#include <dataSR.hpp>
#define MESSAGE2RASERVER 1
#define MESSAGE2STOSTORAGE 2
#define MESSAGE2DEDUPCORE 3

extern Configure config;

DataSR::DataSR()
{
    MQ2DataSR_CallBack_ = new messageQueue<EpollMessage_t>(config.get_EpollMessage_t_MQSize());
    MQ2DedupCore_ = new messageQueue<EpollMessage_t>(config.get_EpollMessage_t_MQSize());
    MQ2StorageCore_ = new messageQueue<EpollMessage_t>(config.get_EpollMessage_t_MQSize());
    MQ2RAServer_ = new messageQueue<EpollMessage_t>(config.get_EpollMessage_t_MQSize());
    socket_.init(SERVER_TCP, "", config.getStorageServerPort());
    epfd = epoll_create(20);
}

DataSR::~DataSR()
{
    delete MQ2DataSR_CallBack_;
    delete MQ2DedupCore_;
    delete MQ2StorageCore_;
    delete MQ2RAServer_;
    socket_.finish();
}

void DataSR::extractMQ()
{
    while (true) {
        EpollMessage_t msgTemp;
        epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        if (extractMQ2DataSR_CallBack(msgTemp)) {
            if (epollSession_.find(msgTemp.fd) == epollSession_.end()) {
                cerr << "find data in epoll session failed" << endl;
                continue;
            } else {
                epollSessionMutex_.lock();
                epollSession_.at(msgTemp.fd).dataSize = msgTemp.dataSize;
                epollSession_.at(msgTemp.fd).type = msgTemp.type;
                // cerr << "DataSR : call back message queue current msg data type = " << msgTemp.type << endl;
                memcpy(epollSession_.at(msgTemp.fd).data, msgTemp.data, msgTemp.dataSize);
                ev.data.fd = msgTemp.fd;
                epoll_ctl(epfd, EPOLL_CTL_MOD, msgTemp.fd, &ev);
                epollSessionMutex_.unlock();
            }
        }
    }
}

bool DataSR::insertMQ(int queueSwitch, EpollMessage_t& msg)
{
    switch (queueSwitch) {
    case MESSAGE2RASERVER: {
        return insertMQ2RAServer(msg);
    }
    case MESSAGE2STOSTORAGE: {
        return insertMQ2StorageCore(msg);
    }
    case MESSAGE2DEDUPCORE: {
        return insertMQ2DedupCore(msg);
    }
    default: {
        cerr << "DataSR insert message queue error : wrong message queue name" << endl;
        return false;
    }
    }
}

void DataSR::run()
{
    epoll_event event[100];
    epoll_event ev;
    ev.data.ptr = nullptr;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = socket_.fd_;
    epoll_ctl(epfd, EPOLL_CTL_ADD, socket_.fd_, &ev);
    u_char buffer[EPOLL_MESSAGE_DATA_SIZE];
    for (int i = 0; i < 100; i++) {
        event[i].data.ptr = nullptr;
    }

    while (true) {
        int nfd = epoll_wait(epfd, event, 20, 500);

        for (int i = 0; i < nfd; i++) {
            //cerr << "Scan for " << nfd << " connections" << endl;
            if (event[i].data.fd == socket_.fd_) {
                Socket tempSock = socket_.Listen();
                socketConnection.insert(map<int, Socket>::value_type(tempSock.fd_, tempSock));

                EpollMessage_t msgTemp;
                msgTemp.fd = tempSock.fd_;
                msgTemp.cid = 0;
                ev.data.ptr = (void*)&msgTemp;
                ev.events = EPOLLET | EPOLLIN;
                ev.data.fd = tempSock.fd_;
                epoll_ctl(epfd, EPOLL_CTL_ADD, tempSock.fd_, &ev);
                epollSessionMutex_.lock();
                epollSession_.insert(make_pair(tempSock.fd_, msgTemp));
                epollSessionMutex_.unlock();
            } else if (event[i].events & EPOLLOUT) {
                // cerr << "Current send event fd = " << event[i].data.fd << endl;

                EpollMessage_t msg1;

                epollSessionMutex_.lock();
                msg1 = epollSession_.at(event[i].data.fd);
                epollSessionMutex_.unlock();
                if (msg1.fd != event[i].data.fd) {
                    cerr << "epoll event fd not equal to msg-fd " << msg1.fd << endl;
                    continue;
                }
                NetworkHeadStruct_t netBody;
                netBody.clientID = msg1.cid;
                netBody.messageType = msg1.type;
                netBody.dataSize = msg1.dataSize;
                memcpy(buffer, &netBody, sizeof(NetworkHeadStruct_t));
                memcpy(buffer + sizeof(NetworkHeadStruct_t), msg1.data, msg1.dataSize);

                if (!socketConnection[event[i].data.fd].Send(buffer, sizeof(NetworkHeadStruct_t) + msg1.dataSize)) {
                    cerr << "DataSR : client closed socket connect, fd = " << event[i].data.fd << endl;
                    ev.data.ptr = nullptr;
                    ev.data.fd = event[i].data.fd;
                    ev.events = EPOLLHUP;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, event[i].data.fd, &ev);
                    epollSessionMutex_.lock();
                    epollSession_.erase(event[i].data.fd);
                    epollSessionMutex_.unlock();
                } else {
                    ev.data.fd = event[i].data.fd;
                    ev.events = EPOLLET | EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, event[i].data.fd, &ev);
                }

            } else if (event[i].events & EPOLLIN) {
                int recvSize = 0;
                if (!socketConnection[event[i].data.fd].Recv(buffer, recvSize)) {
                    cerr << "DataSR : client closed socket connect, fd = " << event[i].data.fd << endl;
                    ev.data.ptr = nullptr;
                    ev.data.fd = event[i].data.fd;
                    ev.events = EPOLLHUP;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, event[i].data.fd, &ev);
                    epollSessionMutex_.lock();
                    epollSession_.erase(event[i].data.fd);
                    epollSessionMutex_.unlock();
                    // EpollMessage_t msg;
                    // msg.type = ERROR_CLIENT_CLOSE_CONNECT;
                    // msg.dataSize = 0;
                    // insertMQ2StorageCore(msg);
                } else {
                    EpollMessage_t msg;
                    ev.data.ptr = (void*)&msg;
                    ev.data.fd = event[i].data.fd;
                    ev.events = EPOLLET | EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, event[i].data.fd, &ev);
                    NetworkHeadStruct_t netBody;
                    memcpy(&netBody, buffer, sizeof(NetworkHeadStruct_t));
                    memcpy(msg.data, buffer + sizeof(NetworkHeadStruct_t), netBody.dataSize);
                    msg.dataSize = netBody.dataSize;
                    msg.cid = netBody.clientID;
                    msg.type = netBody.messageType;
                    msg.fd = event[i].data.fd;
                    cerr << "DataSR : recv message type " << msg.type << ", message size = " << msg.dataSize << endl;
                    switch (netBody.messageType) {
                    case SGX_RA_MSG01: {
                        this->insertMQ(MESSAGE2RASERVER, msg);
                        break;
                    }
                    case SGX_RA_MSG3: {
                        this->insertMQ(MESSAGE2RASERVER, msg);
                        break;
                    }
                    case SGX_SIGNED_HASH: {
                        this->insertMQ(MESSAGE2RASERVER, msg);
                        break;
                    }
                    case CLIENT_UPLOAD_CHUNK: {
                        this->insertMQ(MESSAGE2STOSTORAGE, msg);
                        break;
                    }
                    case CLIENT_UPLOAD_RECIPE: {
                        this->insertMQ(MESSAGE2STOSTORAGE, msg);
                        break;
                    }
                    case CLIENT_DOWNLOAD_FILEHEAD: {
                        this->insertMQ(MESSAGE2STOSTORAGE, msg);
                        break;
                    }
                    case CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE: {
                        this->insertMQ(MESSAGE2STOSTORAGE, msg);
                        break;
                    }
                    default:
                        continue;
                    }
                }
            }
        }
    }
}

bool DataSR::insertMQ2DataSR_CallBack(EpollMessage_t& newMessage)
{
    return MQ2DataSR_CallBack_->push(newMessage);
}

bool DataSR::insertMQ2DedupCore(EpollMessage_t& newMessage)
{
    return MQ2DedupCore_->push(newMessage);
}

bool DataSR::insertMQ2RAServer(EpollMessage_t& newMessage)
{
    return MQ2RAServer_->push(newMessage);
}

bool DataSR::insertMQ2StorageCore(EpollMessage_t& newMessage)
{
    return MQ2StorageCore_->push(newMessage);
}

bool DataSR::extractMQ2DedupCore(EpollMessage_t& newMessage)
{
    return MQ2DedupCore_->pop(newMessage);
}

bool DataSR::extractMQ2RAServer(EpollMessage_t& newMessage)
{
    return MQ2RAServer_->pop(newMessage);
}

bool DataSR::extractMQ2StorageCore(EpollMessage_t& newMessage)
{
    return MQ2StorageCore_->pop(newMessage);
}

bool DataSR::extractMQ2DataSR_CallBack(EpollMessage_t& newMessage)
{
    return MQ2DataSR_CallBack_->pop(newMessage);
}
