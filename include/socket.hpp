#ifndef GENERALDEDUPSYSTEM_NETWORK_HPP
#define GENERALDEDUPSYSTEM_NETWORK_HPP

#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

#define SERVER_TCP 0
#define CLIENT_TCP 1
#define CLIENT_UDP 2
#define SERVER_UDP 3
class Socket {
public:
    int fd_;
    sockaddr_in addr_;
    Socket(const int type, string ip, int port);

    Socket(int fd, sockaddr_in addr);

    Socket(){};

    ~Socket(){};

    void init(const int type, string ip, int port);
    void finish();

    // void setNonBlock();
    bool Send(u_char* buffer, int sendSize);
    bool Recv(u_char* buffer, int& recvSize);

    Socket Listen();
};

#endif //GENERALDEDUPSYSTEM_NETWORK_HPP
