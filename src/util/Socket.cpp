//
// Created by a on 12/14/18.
//

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

Socket::~Socket()
{

    //do not call finish here, more detail in doc
    //this->finish();
}

void Socket::init(const int type, string ip, int port)
{
    memset(&this->addr, 0, sizeof this->addr);
    this->addr.sin_family = AF_INET;
    this->addr.sin_port = htons(port);
    switch (type) {
    case SERVERTCP: {
        this->fd = socket(AF_INET, SOCK_STREAM, 0);
        this->addr.sin_addr.s_addr = htons(INADDR_ANY);
        if (bind(this->fd, (struct sockaddr*)&this->addr, sizeof this->addr) != 0) {
            cerr << "Error at bind fd to Socket\n";
            exit(1);
        }
        listen(this->fd, 10);
        return;
    }
    case CLIENTTCP: {
        this->fd = socket(AF_INET, SOCK_STREAM, 0);
        inet_pton(AF_INET, ip.c_str(), &this->addr.sin_addr);
        if (connect(this->fd, (struct sockaddr*)&this->addr, sizeof this->addr) < 0) {
            cerr << "Can not connect server when init for Client Socket\n";
            cerr << "Socket.cpp::Socket::init\n";
            exit(1);
        }
        return;
    }
    case UDP: {
        this->fd = socket(AF_INET, SOCK_DGRAM, 0);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        if (bind(this->fd, (struct sockaddr*)&this->addr, sizeof this->addr) == -1) {
            std::cerr << "Can not bind to sockfd\n";
            std::cerr << "May cause by shutdown server before client\n";
            std::cerr << "Wait for 30 sec and try again\n";
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

void Socket::finish()
{
    close(this->fd);
}

Socket Socket::Listen()
{
    int cfd;
    sockaddr_in caddr;
    unsigned int addrSize = sizeof caddr;
    cfd = accept(this->fd, (struct sockaddr*)&caddr, &addrSize);
    if (cfd > 0) {
        cerr << "Clinet connect fd : " << cfd << endl;
        return Socket(cfd, caddr);
    }
    cerr << "Error occur when Server accept connection from Client at Socket listen\n";
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
            if (errno == EAGAIN) {
            }
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
            cout << "recv errno: " << errno << endl;
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