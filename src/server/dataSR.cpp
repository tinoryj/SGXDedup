//
// Created by a on 2/3/19.
//

#include <dataSR.hpp>
//TODO:tmp
#include "../../include/dataSR.hpp"
_DataSR::_DataSR()
{
    _inputMQ.createQueue(DATASR_IN_MQ, READ_MESSAGE);
    _mq2DedupCore.createQueue(DATASR_TO_DEDUPCORE_MQ, WRITE_MESSAGE);
    _mq2StorageCore.createQueue(DATASR_TO_STORAGECORE_MQ, WRITE_MESSAGE);
    _mq2RAServer.createQueue(DATASR_TO_POWSERVER_MQ, WRITE_MESSAGE);
    /*
    _outputMQ[0].createQueue(DATASR_TO_DEDUPCORE_MQ,WRITE_MESSAGE);
    _outputMQ[1].createQueue(DATASR_TO_STORAGECORE_MQ,WRITE_MESSAGE);
    _outputMQ[2].createQueue(DATASR_TO_POWSERVER_MQ,WRITE_MESSAGE);
     */
    _socket.init(SERVER_TCP, "", config.getStorageServerPort());
}

_DataSR::~_DataSR()
{
    _socket.finish();
}

bool _DataSR::extractMQ()
{
    epoll_message* msg = new epoll_message();
    ;
    epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    while (1) {
        //        epoll_message *msg=new epoll_message();;
        if (!_inputMQ.pop(*msg)) {
            continue;
        }

        //client had closed,abandon these data
        if (_epollSession.find(msg->_fd) == _epollSession.end()) {
            continue;
        }
        ev.data.ptr = (void*)_epollSession.at(msg->_fd);
        _epollSession.at(msg->_fd)->_data = msg->_data;
        _epollSession.at(msg->_fd)->_type = msg->_type;
        //        ev.data.ptr=(void*)msg;
        epoll_ctl(msg->_epfd, EPOLL_CTL_MOD, msg->_fd, &ev);
    }
}

bool _DataSR::insertMQ(int queueSwitch, epoll_message* msg)
{
    switch (queueSwitch) {
    case MESSAGE2RASERVER: {
        _mq2RAServer.push(*msg);
        break;
    }
    case MESSAGE2STORAGE: {
        _mq2StorageCore.push(*msg);
        break;
    }
    case MESSAGE2DUPCORE: {
        _mq2DedupCore.push(*msg);
    }
    }
}

bool _DataSR::workloadProgress()
{
    int epfd;
    map<int, Socket> socketConnection;
    epoll_event ev, event[100];
    epfd = epoll_create(20);
    epoll_message* msg = new epoll_message();
    msg->_fd = _socket.fd;
    msg->_epfd = epfd;
    ev.data.ptr = (void*)msg;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, msg->_fd, &ev);
    string buffer;
    Socket tmpSock;
    networkStruct netBody(0, 0);

    while (1) {
        int nfd = epoll_wait(epfd, event, 20, -1);
        for (int i = 0; i < nfd; i++) {
            msg = (epoll_message*)event[i].data.ptr;

            //Listen fd
            if (msg->_fd == _socket.fd) {
                epoll_message* msg1 = new epoll_message();
                tmpSock = _socket.Listen();
                socketConnection[tmpSock.fd] = tmpSock;
                msg1->_fd = tmpSock.fd;
                msg1->_epfd = epfd;
                ev.data.ptr = (void*)msg1;
                _epollSession.insert(make_pair(tmpSock.fd, msg1));
                //ev.events = EPOLLET | EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, msg1->_fd, &ev);
                continue;
            }
            if (event[i].events & EPOLLOUT) {
                netBody._data = msg->_data;
                netBody._cid = msg->_cid;
                netBody._type = msg->_type;
                serialize(netBody, buffer);
                if (!socketConnection[msg->_fd].Send(buffer)) {
                    cerr << "perr closed A" << endl;
                    msg->_type = POW_CLOSE_SESSION;
                    msg->_data.clear();
                    epoll_ctl(epfd, EPOLL_CTL_DEL, msg->_fd, &ev);
                    _epollSession.erase(msg->_fd);
                    this->insertMQ(MESSAGE2RASERVER, msg);
                    continue;
                }
                ev.data.ptr = (void*)msg;
                epoll_ctl(epfd, EPOLL_CTL_MOD, msg->_fd, &ev);
                continue;
            }
            if (event[i].events & EPOLLIN) {
                if (!socketConnection[msg->_fd].Recv(buffer)) {
                    cerr << "peer closed B" << endl;
                    cerr << msg->_fd << endl;
                    msg->_type = POW_CLOSE_SESSION;
                    msg->_data.clear();
                    epoll_ctl(epfd, EPOLL_CTL_DEL, msg->_fd, &ev);
                    _epollSession.erase(msg->_fd);
                    this->insertMQ(MESSAGE2RASERVER, msg);
                    continue;
                }
                deserialize(buffer, netBody);
                msg->setNetStruct(netBody);
                switch (netBody._type) {
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
                    this->insertMQ(MESSAGE2DUPCORE, msg);
                    break;
                }
                case CLIENT_UPLOAD_RECIPE: {
                    this->insertMQ(MESSAGE2STORAGE, msg);
                    break;
                }
                case CLIENT_DOWNLOAD_RECIPE: {
                    this->insertMQ(MESSAGE2STORAGE, msg);
                    break;
                } /*
                    case CLIENT_DOWNLOAD_CHUNK:{

                    }*/
                default:
                    continue;
                }
            }
        }
    }
}

void dataSR::run()
{
    boost::thread th(boost::bind(&_DataSR::extractMQ, this));
    this->workloadProgress();
    th.detach();
}

bool dataSR::receiveData() {}
bool dataSR::sendData() {}