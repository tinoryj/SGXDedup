#ifndef GENERALDEDUPSYSTEM_SSL_HPP
#define GENERALDEDUPSYSTEM_SSL_HPP

#include <iostream>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"
#include <bits/stdc++.h>

#define SERVERSIDE 0
#define CLIENTSIDE 1

#define SECRT "key/sslKeys/server-cert.pem"
#define SEKEY "key/sslKeys/server-key.pem"
#define CLCRT "key/sslKeys/client-cert.pem"
#define CLKEY "key/sslKeys/client-key.pem"
#define CACRT "key/sslKeys/ca-cert.pem"

using namespace std;

class ssl {
private:
    SSL_CTX* ctx_;
    struct sockaddr_in sockAddr_;
    std::string serverIP_;
    int port_;
    //    std::vector<int>_fdList;
    //    std::vector<SSL*>_sslList;

public:
    int listenFd_;
    ssl(std::string ip, int port, int scSwitch);
    ~ssl();
    std::pair<int, SSL*> sslConnect();
    std::pair<int, SSL*> sslListen();
    //std::pair<int,SSL*> sslConnect();
    //std::pair<int,SSL*> sslListen();
    void sslWrite(SSL* connection, std::string data);
    bool sslRead(SSL* connection, std::string& data);
};

#endif //GENERALDEDUPSYSTEM_SSL_HPP
