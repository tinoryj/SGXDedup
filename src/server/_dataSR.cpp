#include "_dataSR.hpp"

_DataSR::_DataSR() {
    _inputMQ.createQueue("dupCore to DataSR",READ_MESSAGE);
    _outputMQ[0].createQueue("DataSR to dupCore",WRITE_MESSAGE);
    _outputMQ[1].createQueue("DataSR to Storage System",WRITE_MESSAGE);
    _outputMQ[2].createQueue("DataSR to RA Server",WRITE_MESSAGE);
    _socket.init(SERVERTCP,"",0);
}

_DataSR::~_DataSR() {
    _socket.finish();
}

bool _DataSR::extractMQ() {
    epoll_message *msg=new epoll_message();;
    epoll_event ev;
    ev.events=EPOLLIN|EPOLLET;
    while(1){
        _inputMQ.pop(*msg);
        ev.data.ptr=(void*)msg;
        epoll_ctl(msg->_epfd,EPOLL_CTL_MOD,msg->_fd,&ev);
    }
}

bool _DataSR::insertMQ(int queueSwitch, epoll_message *msg) {
    _outputMQ->push(*msg);
    delete msg;
}

bool _DataSR::workloadProgress() {
    int epfd;
    map<int,Sock> socketConnection;
    epoll_event ev,event[100];
    epfd=epoll_create(20);
    epoll_message *msg=new epoll_message();
    msg->_fd=_socket.fd;
    msg->_epfd=epfd;
    ev.data.ptr=(void*)msg;
    ev.events=EPOLLIN|EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,msg->_fd,&ev);
    while(1){
        int nfd=epoll_wait(epfd,event,20,-1);
        for(int i=0;i<nfd;i++){
            msg=(epoll_message*)event[i].data.ptr;
            if(msg->_fd==_socket.fd){
                epoll_message *msg1=new epoll_message();
                Sock tmpSock=_socket.Listen();
                socketConnection[tmpSock.fd]=tmpSock;
                msg1->_fd=tmpSock.fd;
                ev.data.ptr=(void*)msg1;
                ev.events=EPOLLET|EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,msg1->_fd,&ev);
                continue;
            }
            if(event[i].events&EPOLLOUT){
                socketConnection[msg->_fd].Send(msg->_data);
                delete msg;
                continue;
            }
            if(event[i].events&EPOLLIN){
                socketConnection[msg->_fd].Recv(msg->_data);
                switch(msg->_data[0]){
                    case SGX_RA_MSG01:{
                        this->insertMQ(MESSAGE2RASERVER,msg);
                        break;
                    }
                    case SGX_RA_MSG3:{
                        this->insertMQ(MESSAGE2RASERVER,msg);
                        break;
                    }
                    case SGX_SIGNED_HASH:{
                        this->insertMQ(MESSAGE2DUPCORE,msg);
                        break;
                    }
                    case CLIENT_UPLOAD_CHUNK:{
                        this->insertMQ(MESSAGE2DUPCORE,msg);
                        break;
                    }
                    case CLIENT_UPLOAD_RECIPE:{
                        this->insertMQ(MESSAGE2STORAGE,msg);
                        break;
                    }
                    default:
                        continue;
                }
            }
        }
    }
}