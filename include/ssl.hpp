#ifndef SGXDEDUP_SSL_HPP
#define SGXDEDUP_SSL_HPP

#include "configure.hpp"
#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVERSIDE 0
#define CLIENTSIDE 1

#define SECRT "key/sslKeys/server.crt"
#define SEKEY "key/sslKeys/server_rsa_private.pem"
#define CLCRT "key/sslKeys/client.crt"
#define CLKEY "key/sslKeys/client_rsa_private.pem"
#define CACRT "key/sslKeys/ca.crt"

using namespace std;

class ssl {
private:
    SSL_CTX* _ctx;
    struct sockaddr_in _sockAddr;
    std::string _serverIP;
    int _port;

public:
    int listenFd;
    ssl(std::string ip, int port, int scSwitch);
    ~ssl();
    std::pair<int, SSL*> sslConnect();
    std::pair<int, SSL*> sslListen();
    bool send(SSL* connection, char* data, int dataSize);
    bool recv(SSL* connection, char* data, int& dataSize);
    bool getSSLErrorMessage(SSL* connection, int ret);
};
#endif //SGXDEDUP_SSL_HPP
