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

#define SERVERTCP 0
#define CLIENTTCP 1
#define UDP 2

class Socket {
private:
    sockaddr_in addr;

public:
    int fd;

    Socket(const int type, string ip, int port);

    Socket(int fd, sockaddr_in addr);

    Socket(){};

    ~Socket(){};

    void init(const int type, string ip, int port);

    void finish();

    // void setNonBlock();
    bool Send(const string buffer);

    bool Recv(string& buffer);

    Socket Listen();

    bool printErrnoMessage(auto errno);
};

#endif //GENERALDEDUPSYSTEM_NETWORK_HPP
