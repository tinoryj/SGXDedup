#include "Socket.hpp"
#include <iomanip>
#include <iostream>
using namespace std;

Socket::Socket(const int type, string ip, int port)
{
    this->init(type, ip, port);
}

Socket::Socket(int fd, sockaddr_in addr)
    : fd(fd)
    , addr(addr)
{
}

Socket::Socket() {}

void Socket::init(const int type, string ip, int port)
{
    memset(&this->addr, 0, sizeof(this->addr));
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    switch (type) {
    case SERVER_TCP: {
        this->fd = socket(AF_INET, SOCK_STREAM, 0);
        this->addr.sin_addr.s_addr = htons(INADDR_ANY);
        if (bind(this->fd, (struct sockaddr*)&this->addr, sizeof this->addr) != 0) {
            cerr << "Error at bind fd to Socket" << endl;
            exit(1);
        }
        listen(this->fd, 10);
        return;
    }
    case CLIENT_TCP: {
        this->fd = socket(AF_INET, SOCK_STREAM, 0);
        inet_pton(AF_INET, ip.c_str(), &this->addr.sin_addr);
        if (connect(this->fd, (struct sockaddr*)&this->addr, sizeof this->addr) < 0) {
            cerr << "Can not connect server when init for Client Socket" << endl;
            cerr << "Socket.cpp::Socket::init" << endl;
            exit(1);
        }
        return;
    }
    case UDP: {
        this->fd = socket(AF_INET, SOCK_DGRAM, 0);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        if (bind(this->fd, (struct sockaddr*)&this->addr, sizeof this->addr) == -1) {
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
    close(this->fd);
}

Socket Socket::Listen()
{
    int clientFd;
    sockaddr_in clientAddr;
    uint addrSize = sizeof(clientAddr);
    clientFd = accept(this->fd, (struct sockaddr*)&clientAddr, &addrSize);
    if (clientFd > 0) {
        cerr << "Clinet connect fd : " << clientFd << endl;
        return Socket(clientFd, clientAddr);
    }
    cerr << "Error occur when Server accept connection from Client at Socket listen" << endl;
    exit(1);
}

bool Socket::Send(const string buffer)
{
    int sentSize = 0, sendSize = buffer.length();
    int len;
    size_t s = write(this->fd, (char*)&sendSize, sizeof(int));
    if (s < 0) {
        cerr << "send errno: " << errno << endl;
        this->finish();
        return false;
    }
    while (sentSize < sendSize) {
        len = write(this->fd, buffer.c_str() + sentSize, buffer.length() - sentSize);
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

bool Socket::Recv(string& buffer)
{
    cerr << setbase(10);
    size_t recvedSize = 0, s;
    int recvSize;
    buffer.clear();
    s = read(this->fd, (char*)&recvSize, sizeof(int));
    if (s == 0) {
        this->finish();
        return false;
    }
    if (recvedSize < 0) {
        cerr << "Socket: recvSize: " << setbase(10) << recvedSize << endl;
        exit(0);
    }
    buffer.resize(recvSize);

    while (recvedSize < recvSize) {
        s = read(this->fd, &buffer[recvedSize], recvSize - recvedSize);
        //should check errno here
        if (s <= 0) {
            cerr << "recv errno: " << errno << endl;
            this->finish();
            return false;
        }
        recvedSize += s;
    }
    if (recvedSize != recvSize) {
        this->finish();
        return false;
    }

    return true;
}
