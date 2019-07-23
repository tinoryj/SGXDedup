#include "socket.hpp"
#include <iomanip>
#include <iostream>
using namespace std;

Socket::Socket(const int type, string ip, int port)
{
    this->init(type, ip, port);
}

Socket::Socket(int fd, sockaddr_in addr)
    : fd_(fd)
    , addr_(addr)
{
}

Socket::Socket() {}

void Socket::init(const int type, string ip, int port)
{
    memset(&this->addr_, 0, sizeof(this->addr_));
    this->addr_.sin_family = AF_INET;
    this->addr_.sin_port = htons(port);
    switch (type) {
    case SERVER_TCP: {
        this->fd_ = socket(AF_INET, SOCK_STREAM, 0);
        this->addr_.sin_addr.s_addr = htons(INADDR_ANY);
        if (bind(this->fd_, (struct sockaddr*)&this->addr_, sizeof this->addr_) != 0) {
            cerr << "Error at bind fd to Socket" << endl;
            exit(1);
        }
        listen(this->fd_, 10);
        return;
    }
    case CLIENT_TCP: {
        this->fd_ = socket(AF_INET, SOCK_STREAM, 0);
        inet_pton(AF_INET, ip.c_str(), &this->addr_.sin_addr);
        if (connect(this->fd_, (struct sockaddr*)&this->addr_, sizeof this->addr_) < 0) {
            cerr << "Can not connect server when init for Client Socket" << endl;
            cerr << "Socket.cpp::Socket::init" << endl;
            exit(1);
        }
        return;
    }
    case UDP: {
        this->fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
        if (bind(this->fd_, (struct sockaddr*)&this->addr_, sizeof this->addr_) == -1) {
            std::cerr << "Can not bind to sockfd" << endl;
            std::cerr << "May cause by shutdown server before client" << endl;
            std::cerr << "Wait for 30 sec and try again" << endl;
            exit(1);
        }
        return;
    }
    default: {
        cerr << "Type Error at Socket(const int type, string ip, int port)" << endl;
        cerr << "Type supported SERVER_TCP-0 CLIENT_TCP-1 UDP-2" << endl;
        exit(1);
    }
    }
}

void Socket::finish()
{
    close(this->fd_);
}

Socket Socket::Listen()
{
    int clientFd;
    sockaddr_in clientAddr;
    uint addrSize = sizeof(clientAddr);
    clientFd = accept(this->fd_, (struct sockaddr*)&clientAddr, &addrSize);
    if (clientFd > 0) {
        cerr << "Clinet connect fd : " << clientFd << endl;
        return Socket(clientFd, clientAddr);
    }
    cerr << "Error occur when Server accept connection from Client at Socket listen" << endl;
    exit(1);
}

bool Socket::Send(u_char* buffer, int sendSize)
{
    int sentSize = 0;
    int len;
    size_t s = write(this->fd_, (char*)&sendSize, sizeof(int));
    if (s < 0) {
        cerr << "send errno: " << errno << endl;
        this->finish();
        return false;
    }
    while (sentSize < sendSize) {
        len = write(this->fd_, buffer + sentSize, sendSize - sentSize);
        //should check errno here
        if (len <= 0) {
            cerr << "send errno: " << errno << endl;
        }
        sentSize += len;
    }
    if (sentSize != sendSize) {
        this->finish();
        return false;
    }
    return true;
}

bool Socket::Recv(u_char* buffer, int& recvSize)
{
    size_t recvedSize = 0;
    size_t readByteCount;
    readByteCount = read(this->fd_, (char*)&recvSize, sizeof(int));
    if (readByteCount == 0) {
        this->finish();
        return false;
    }
    if (recvedSize < 0) {
        cerr << "Socket: recvSize: " << setbase(10) << recvedSize << endl;
        exit(0);
    }

    while (recvedSize < recvSize) {
        readByteCount = read(this->fd_, buffer + recvedSize, recvSize - recvedSize);
        //should check errno here
        if (readByteCount <= 0) {
            cerr << "recv errno: " << errno << endl;
            this->finish();
            return false;
        }
        recvedSize += readByteCount;
    }
    if (recvedSize != recvSize) {
        this->finish();
        return false;
    }
    return true;
}
