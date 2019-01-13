//
// Created by a on 12/14/18.
//

#include "Sock.hpp"
using namespace std;

Sock::Sock(const int type, string ip, int port) {
    this->init(type,ip,port);
}

Sock::Sock(int fd, sockaddr_in addr):fd(fd),addr(addr) {}

Sock::Sock() {}

Sock::~Sock() {
    this->finish();
}

void Sock::init(const int type, string ip, int port) {
    memset(&this->addr,0,sizeof this->addr);
    this->addr.sin_family=AF_INET;
    this->addr.sin_port=htons(port);
    switch (type){
        case SERVERTCP:{
            this->fd=socket(AF_INET,SOCK_STREAM,0);
            this->addr.sin_addr.s_addr=htons(INADDR_ANY);
            if(bind(this->fd,(struct sockaddr*)&this->addr,sizeof this->addr)!=0){
                cerr<<"Error at bind fd to socket\n";
                exit(1);
            }
            listen(this->fd,10);
            return;
        }
        case CLIENTTCP:{
            this->fd=socket(AF_INET,SOCK_STREAM,0);
            inet_pton(AF_INET,ip.c_str(),&this->addr.sin_addr);
            if(connect(this->fd,(struct sockaddr*)&this->addr,sizeof this->addr)<0) {
                cerr << "Can not connect server when init for Client Socket\n";
                exit(1);
            }
            return;
        }
        case UDP:{
            this->fd=socket(AF_INET,SOCK_DGRAM,0);
            inet_pton(AF_INET,ip.c_str(),&addr.sin_addr);
            if(bind(this->fd,(struct sockaddr*)&this->addr,sizeof this->addr)!=0){
                cerr<<"Error at bind fd to socket\n";
                exit(1);
            }
            return;
        }
        default:{
            cerr<<"Type Error at Sock(const int type, string ip, int port)\n";
            cerr<<"Type supported SERVERTCP-0 CLIENTTCP-1 UDP-2\n";
            exit(1);
        }
    }
}

void Sock::finish() {
    close(this->fd);
}

Sock Sock::Listen() {
    int cfd;
    sockaddr_in caddr;
    unsigned int addrSize=sizeof caddr;
    cfd=accept(this->fd,(struct sockaddr*)&caddr,&addrSize);
    if(cfd>0){
        return Sock(cfd,caddr);
    }
    cerr<<"Error occur when Server accept connection from Client at Socket listen\n";
    exit(1);
}

bool Sock::Send(const string buffer) {
    int sentSize=0,cnt=0,sendSize=buffer.length();
    write(this->fd,(char*)&sendSize,sizeof(int));
    while(sentSize<sendSize&&cnt<5){
        cnt++;
        sentSize+=write(this->fd,buffer.c_str()+sentSize,buffer.length()-sentSize);
    }
    return sentSize==sendSize;
}

bool Sock::Recv(string &buffer) {
    size_t recvedSize=0,len,cnt=0,s;
    buffer.clear();
    s=read(this->fd,(char*)&len,sizeof(int));
    if(s==0){
        this->finish();
        return false;
    }
    buffer.resize(len);

    while(recvedSize<len){
        recvedSize+=read(this->fd,&buffer[recvedSize],len-recvedSize);
    }
    return true;
}