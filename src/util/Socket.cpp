//
// Created by a on 12/14/18.
//

#include "Socket.hpp"
using namespace std;

Socket::Socket(const int type, string ip, int port) {
    this->init(type,ip,port);
}

Socket::Socket(int fd, sockaddr_in addr):fd(fd),addr(addr) {}

Socket::Socket() {}

Socket::~Socket() {
    //this->finish();
}

void Socket::init(const int type, string ip, int port) {
    memset(&this->addr, 0, sizeof this->addr);
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    switch (type) {
        case SERVERTCP: {
            this->fd = socket(AF_INET, SOCK_STREAM, 0);
            this->addr.sin_addr.s_addr = htons(INADDR_ANY);
            if (bind(this->fd, (struct sockaddr *) &this->addr, sizeof this->addr) != 0) {
                cerr << "Error at bind fd to Socket\n";
                exit(1);
            }
            listen(this->fd, 10);
            return;
        }
        case CLIENTTCP: {
            this->fd = socket(AF_INET, SOCK_STREAM, 0);
            inet_pton(AF_INET, ip.c_str(), &this->addr.sin_addr);
            if (connect(this->fd, (struct sockaddr *) &this->addr, sizeof this->addr) < 0) {
                cerr << "Can not connect server when init for Client Socket\n";
                cerr << "Socket.cpp::Socket::init\n";
                exit(1);
            }
            return;
        }
        case UDP: {
            this->fd = socket(AF_INET, SOCK_DGRAM, 0);
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
            if (bind(this->fd, (struct sockaddr *) &this->addr, sizeof this->addr) ==-1) {
                std::cerr<<"Can not bind to sockfd\n";
                std::cerr<<"May cause by shutdown server before client\n";
                std::cerr<<"Wait for 30 sec and try again\n";
                exit(1);
            }
            return;
        }
        default: {
            cerr << "Type Error at Socket(const int type, string ip, int port)\n";
            cerr << "Type supported SERVERTCP-0 CLIENTTCP-1 UDP-2\n";
            exit(1);
        }
    }
}

void Socket::finish() {
    close(this->fd);
}

Socket Socket::Listen() {
    int cfd;
    sockaddr_in caddr;
    unsigned int addrSize=sizeof caddr;
    cfd=accept(this->fd,(struct sockaddr*)&caddr,&addrSize);
    if(cfd>0){
        cerr<<"Clinet connect fd : "<<cfd<<endl;
        return Socket(cfd,caddr);
    }
    cerr<<"Error occur when Server accept connection from Client at Socket listen\n";
    exit(1);
}

bool Socket::Send(const string buffer) {
    int sentSize = 0, cnt = 0, sendSize = buffer.length();
    int len;
    write(this->fd, (char *) &sendSize, sizeof(int));
    while (sentSize < sendSize && cnt < 5) {
        cnt++;
        len = write(this->fd, buffer.c_str() + sentSize, buffer.length() - sentSize);
        //should check errno here
        if (len <= 0) return false;
        sentSize += len;
    }
    return (sentSize == sendSize);
}

bool Socket::Recv(string &buffer) {
    size_t recvedSize = 0, cnt = 0, s;
    int recvSize;
    buffer.clear();
    s = read(this->fd, (char *) &recvSize, sizeof(int));
    if (s == 0) {
        this->finish();
        return false;
    }
    buffer.resize(recvSize);

    while (recvedSize < recvSize && cnt < 5) {
        cnt++;
        s = read(this->fd, &buffer[recvedSize], recvSize - recvedSize);
        //should check errno here
        if (s <= 0) return false;
        recvedSize += s;
    }
    return (recvedSize == recvSize);
}