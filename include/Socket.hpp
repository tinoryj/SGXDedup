//
// Created by a on 12/14/18.
//

#ifndef GENERALDEDUPSYSTEM_NETWORK_HPP
#define GENERALDEDUPSYSTEM_NETWORK_HPP

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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

    Socket();

    ~Socket();

    void init(const int type, string ip, int port);

    void finish();

    // void setNonBlock();
    bool Send(const string buffer);

    bool Recv(string &buffer);

    Socket Listen();
};

/*void Socket::setNonBlock() {

}*/

#endif //GENERALDEDUPSYSTEM_NETWORK_HPP
