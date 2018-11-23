//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_SSL_HPP
#define GENERALDEDUPSYSTEM_SSL_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define SECRT "server.crt"
#define SEKEY "server.key"
#define CLCRT "client.crt"
#define CLKEY "client.key"
#define CACRT "ca.crt"

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

struct connection{
    SSL* ssl;
    int fd;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & ssl;
        ar & fd;
    }
};

class ssl{
private:
    SSL_CTX* _ctx;
    struct sockaddr_in _sockAddr;
    std::string _serverIP;
    int port;
//    std::vector<int>_fdList;
//    std::vector<SSL*>_sslList;

public:
    int listenFd;
    ssl(std::string ip,int port,int scSwitch);
    ~ssl();
    connection sslConnect();
    connection sslListen();
    void sslWrite(SSL* connection,std::string data);
    void sslRead(SSL* connection,std::string& data);
};


#endif //GENERALDEDUPSYSTEM_SSL_HPP
