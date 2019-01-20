//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_SSL_HPP
#define GENERALDEDUPSYSTEM_SSL_HPP

#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#define SERVERSIDE 0
#define CLIENTSIDE 1

#define SECRT "key/servercert.pem"
#define SEKEY "key/server.key"
#define CLCRT "key/clientcert.pem"
#define CLKEY "key/client.key"
#define CACRT "key/cacert.pem"

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

class ssl{
private:
    SSL_CTX* _ctx;
    struct sockaddr_in _sockAddr;
    std::string _serverIP;
    int _port;
//    std::vector<int>_fdList;
//    std::vector<SSL*>_sslList;

public:
    int listenFd;
    ssl(std::string ip,int port,int scSwitch);
    ~ssl();
    std::pair<int,SSL*> sslConnect();
    std::pair<int,SSL*> sslListen();
    //std::pair<int,SSL*> sslConnect();
    //std::pair<int,SSL*> sslListen();
    void sslWrite(SSL* connection,std::string data);
    void sslRead(SSL* connection,std::string& data);
};


#endif //GENERALDEDUPSYSTEM_SSL_HPP
