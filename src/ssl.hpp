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

using namespace std;

class ssl{
private:
	SSL_CTX* _ctx;
	struct sockaddr_in _sockAddr;
	string serverIP;
	int port;
	
	int listenFd;

	vector<int>fdList;
	vector<SSL*>SSLList;

public:
	ssl(string ip,int port,int scSwitch);
	~ssl();
	SSL* sslConnect();
	SSL* sslListen();
};

ssl::ssl(string ip,int port,int scSwitch){
	this->serverIP=ip;
	this->port=port;

	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();
	memset(&_sockAddr,0,sizeof(_sockAddr));
	SSL_METHOD* meth;
	string keyFile,crtFile;

	_sockAddr.sin_port=htons(port);
	_sockAddr.sin_family=AF_INET; 

	switch(scSwitch){
		case SERVERSIDE:{
			meth=TLSv1_server_method();
			keyFile="server.key";
			crtFile="server.crt"
			_sockAddr.sin_addr.s_addr=htons(INADDR_ANY);
			bind(listenFd,(sockaddr*)*_sockAddr,sizeof(_sockAddr));
			listen(listenFd,10);
		}
		case CLIENTSIDE:{
			meth=TLSv1_client_method();
			keyFile="client.key";
			crtFile="client.crt";
			_sockAddr.sin_addr.s_addr=inet_addr(ip);
		};
	}

	_ctx=SSL_CTX_new(meth);
	SSL_CTX_set_verify(_ctx,SSL_VERIFY_PEER,NULL);
	SSL_CTX_load_verify_locations(_ctx,"ca.crt",NULL);
	SSL_CTX_use_certificate_file(_ctx,crtFile,SSL_FILE_TYPE_PEM);
	SSL_CTX_use_PrivateKey_file(_ctx,keyFile,SSL_FILE_TYPE_PEM);
	if(!SSL_CTX_check_private_key(_ctx)){
		cerr<<"1\n";
		exit(1);
	}
}

ssl::~ssl(){
	for(auto fd:fdList){
		close(fd);
	}
	for(auto sslConection:SSLList){
		SSL_shutdown(sslConection);
		SSL_free(sslConection);
	}
}

SSL* ssl::sslConnect(){
	int fd;
	SSL* sslConection;

	fd=socket(AF_INET.SOCK_STREAM,0);
	connect(fd,(struct sockaddr*)&_sockAddr,sizeof(sockaddr));
	sslConection=SSL_new(_ctx);
	SSL_set_fd(sslConection,fd);
	SSL_connect(sslConection);

	fdList.push_back(fd);
	SSLList.public(sslConection);
	return sslConection
}

SSL* ssl:sslListen(){
	int fd;
	while(fd=accept(listenFd,(struct sockaddr*)NULL,NULL)){
		SSL* sslConection=SSL_new(_ctx);
		SSL_set_fd(sslConection,fd);

		fdList.push_back(fd);
		SSLList.push_back(sslConection);
		return sslConection;
	}
	return (SSL*)NULL;
}

#endif

#endif