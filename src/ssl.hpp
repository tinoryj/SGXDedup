#ifndef SSL_HPP
#define SSL_HPP


#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <resolv.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>

#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

#define SERVERSIDE 0
#define CLIENTSIDE 1

using namespace std;

class ssl{
private:
	SSL_CTX* _ctx;
	struct sockaddr_in _sockAddr;
	string serverIP;
	int port;
	
	int listenFd;

public:
	ssl(string ip,int port,int scSwitch);
	~ssl();
	SSL* sslConnect();
	SSL* sslListen();
	void sslWrite(SSL* connection,string data);
	void sslRead(SSL* connection,string& data);
};

#endif

#endif