#include <dataSR.hpp>
dataSR::dataSR()
{
    _socket.init(SERVER_TCP, "", config.getStorageServerPort());
}

dataSR::~dataSR()
{
    _socket.finish();
}

bool dataSR::extractMQ()
{
    EpollMessage_t msg;
    epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    while (1) {
        //        EpollMessage_t *msg=new EpollMessage_t();;
        if (!_inputMQ.pop(*msg)) {
            continue;
        }

        //client had closed,abandon these data
        if (_epollSession.find(msg->_fd) == _epollSession.end()) {
            continue;
        }
        ev.data.ptr = (void*)_epollSession.at(msg->_fd);
        _epollSession.at(msg->fd)->_data = msg->data;
        _epollSession.at(msg->fd)->_type = msg->type;
        //        ev.data.ptr=(void*)msg;
        epoll_ctl(msg->epfd, EPOLL_CTL_MOD, msg->fd, &ev);
    }
}

bool dataSR::insertMQ(int queueSwitch, EpollMessage_t msg)
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

bool dataSR::workloadProgress()
{
    int epfd;
    map<int, Socket> socketConnection;
    epoll_event ev, event[100];
    epfd = epoll_create(20);
    EpollMessage_t msg;
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
            msg = (EpollMessage_t*)event[i].data.ptr;

            //Listen fd
            if (msg->_fd == _socket.fd) {
                EpollMessage_t msg1;
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
    boost::thread th(boost::bind(&dataSR::extractMQ, this));
    this->workloadProgress();
    th.detach();
}

bool dataSR::receiveData() {}
bool dataSR::sendData() {}