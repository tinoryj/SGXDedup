#ifndef SSL_HPP
#define SSL_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#define SERVERSIDE 0
#define CLIENTSIDE 1

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

class ssl{
private:
	SSL_CTX* _ctx;
	struct sockaddr_in _sockAddr;
	std::string _serverIP;
	int port;
	std::vector<int>_fdList;
	std::vector<SSL*>_sslList;
	
	int listenFd;

public:
	ssl(std::string ip,int port,int scSwitch);
	~ssl();
	SSL* sslConnect();
	SSL* sslListen();
	void sslWrite(SSL* connection,std::string data);
	void sslRead(SSL* connection,std::string& data);
};

#endif
