#include <dataSR.hpp>
#define MESSAGE2RASERVER 1
#define MESSAGE2STORAGE 2
#define MESSAGE2DEDUPCORE 3

DataSR::DataSR()
{
    socket_.init(SERVER_TCP, "", config.getStorageServerPort());
}

DataSR::~DataSR()
{
    socket_.finish();
}

void DataSR::extractMQ()
{
    while (true) {
        EpollMessage_t msg;
        epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        if (!extractMQ2DataSR_CallBack(msg)) {
            continue;
        }
        //client had closed,abandon these data
        if (epollSession_.find(msg.fd) == epollSession_.end()) {
            continue;
        }
        ev.data.ptr = (void*)&epollSession_.at(msg.fd);
        memcpy(epollSession_.at(msg.fd).data, msg.data, msg.dataSize);
        epollSession_.at(msg.fd).type = msg.type;
        epoll_ctl(msg.epfd, EPOLL_CTL_MOD, msg.fd, &ev);
    }
}

bool DataSR::insertMQ(int queueSwitch, EpollMessage_t& msg)
{
    switch (queueSwitch) {
    case MESSAGE2RASERVER: {
        return insertMQ2RAServer(msg);
    }
    case MESSAGE2STORAGE: {
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
    int epfd;
    map<int, Socket> socketConnection;
    epoll_event ev, event[100];
    epfd = epoll_create(20);
    EpollMessage_t msg;
    msg.fd = socket_.fd_;
    msg.epfd = epfd;
    ev.data.ptr = &msg;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, msg.fd, &ev);
    u_char buffer[sizeof(EpollMessage_t)];
    Socket tmpSock;

    NetworkHeadStruct_t netBody;
    netBody.clientID = 0;
    netBody.messageType = 0;
    netBody.dataSize = 0;
    cerr << "DataSR : main thread start" << endl;
    while (true) {
        int nfd = epoll_wait(epfd, event, 20, -1);
        for (int i = 0; i < nfd; i++) {

            // if (event[i].data.fd == socket_.fd_) {
            //     Socket tempSock = socket_.Listen();
            //     ev.data.fd = tempSock.fd_;
            //     ev.events = EPOLLET | EPOLLIN;
            //     epoll_ctl(epfd, EPOLL_CTL_ADD, tempSock.fd_, &ev);
            // } else if (event[i].events & EPOLLIN) {
            // }
            memcpy(&msg, event[i].data.ptr, sizeof(EpollMessage_t));

            //Listen fd
            if (msg.fd == socket_.fd_) {
                EpollMessage_t msg1;
                tmpSock = socket_.Listen();
                cerr << "DataSR : recv connection at " << tmpSock.fd_ << endl;
                socketConnection[tmpSock.fd_] = tmpSock;
                msg1.fd = tmpSock.fd_;
                msg1.epfd = epfd;
                ev.data.ptr = (void*)&msg1;
                epollSession_.insert(make_pair(tmpSock.fd_, msg1));
                ev.events = EPOLLET | EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, msg1.fd, &ev);
                continue;
            }
            if (event[i].events & EPOLLOUT) {
                netBody.clientID = msg.cid;
                netBody.messageType = msg.type;
                netBody.dataSize = msg.dataSize;
                memcpy(buffer, &netBody, sizeof(NetworkHeadStruct_t));
                memcpy(buffer + sizeof(NetworkHeadStruct_t), msg.data, msg.dataSize);
                if (!socketConnection[msg.fd].Send(buffer, sizeof(NetworkHeadStruct_t) + msg.dataSize)) {
                    cerr << "perr closed A" << endl;
                    msg.type = POW_CLOSE_SESSION;
                    memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, msg.fd, &ev);
                    epollSession_.erase(msg.fd);
                    this->insertMQ(MESSAGE2RASERVER, msg);
                    continue;
                }
                cerr << "DataSR : send message type " << msg.type << endl;
                ev.data.ptr = (void*)&msg;
                epoll_ctl(epfd, EPOLL_CTL_MOD, msg.fd, &ev);
                continue;
            }
            int recvSize;
            if (event[i].events & EPOLLIN) {
                if (!socketConnection[msg.fd].Recv(buffer, recvSize)) {
                    cerr << "peer closed B" << endl;
                    cerr << msg.fd << endl;
                    msg.type = POW_CLOSE_SESSION;
                    memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, msg.fd, &ev);
                    epollSession_.erase(msg.fd);
                    this->insertMQ(MESSAGE2RASERVER, msg);
                    continue;
                }
                memcpy(&netBody, buffer, sizeof(NetworkHeadStruct_t));
                memcpy(msg.data, buffer + sizeof(NetworkHeadStruct_t), netBody.dataSize);
                msg.dataSize = netBody.dataSize;
                msg.cid = netBody.clientID;
                msg.type = netBody.messageType;
                cerr << "DataSR : recv message type " << msg.type << endl;
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
                    this->insertMQ(MESSAGE2DEDUPCORE, msg);
                    break;
                }
                case CLIENT_UPLOAD_RECIPE: {
                    this->insertMQ(MESSAGE2STORAGE, msg);
                    break;
                }
                case CLIENT_DOWNLOAD_FILEHEAD: {
                    this->insertMQ(MESSAGE2STORAGE, msg);
                    break;
                }
                case CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE: {
                    this->insertMQ(MESSAGE2STORAGE, msg);
                    break;
                }
                default:
                    continue;
                }
            }
        }
    }
}

bool DataSR::insertMQ2DataSR_CallBack(EpollMessage_t& newMessage)
{
    return MQ2DataSR_CallBack_.push(newMessage);
}

bool DataSR::insertMQ2DedupCore(EpollMessage_t& newMessage)
{
    return MQ2DedupCore_.push(newMessage);
}

bool DataSR::insertMQ2RAServer(EpollMessage_t& newMessage)
{
    return MQ2RAServer_.push(newMessage);
}

bool DataSR::insertMQ2StorageCore(EpollMessage_t& newMessage)
{
    return MQ2StorageCore_.push(newMessage);
}

bool DataSR::extractMQ2DedupCore(EpollMessage_t& newMessage)
{
    return MQ2DedupCore_.pop(newMessage);
}

bool DataSR::extractMQ2RAServer(EpollMessage_t& newMessage)
{
    return MQ2RAServer_.pop(newMessage);
}

bool DataSR::extractMQ2StorageCore(EpollMessage_t& newMessage)
{
    return MQ2StorageCore_.pop(newMessage);
}

bool DataSR::extractMQ2DataSR_CallBack(EpollMessage_t& newMessage)
{
    return MQ2DataSR_CallBack_.pop(newMessage);
}

// bool DataSR::extractTimerMQToStorageCore(StorageCoreData_t& newData)
// {
//     return timerObj_->extractMQToStorageCore(newData);
// }
// bool DataSR::insertTimerMQToStorageCore(StorageCoreData_t& newData)
// {
//     return timerObj_->insertMQToStorageCore(newData);
// }