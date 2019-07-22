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

#define SERVER_CERT "key/servercert.pem"
#define SERVER_KEY "key/server.key"
#define CLIENT_CERT "key/clientcert.pem"
#define CLIENT_KEY "key/client.key"
#define CA_CERT "key/cacert.pem"

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
