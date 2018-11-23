#include <bits/stdc++.h>
#include <gmpxx.h>
#include <openssl/aes.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <openssl/ssl.h>
#include "openssl/rsa.h"      
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/err.h"
using namespace std;
int main(){
	int serverfd,tfd;
	SSL_CTX* ctx;
	struct sockaddr_in _sock;

	SSL_load_error_strings();
	OpenSSL_add_ssl_algorithms();

	ctx=SSL_CTX_new(TLSv1_server_method());

	SSL_CTX_set_mode(ctx,SSL_MODE_AUTO_RETRY);
	SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);
	//SSL_set_verify_depth(ctx,1);
	SSL_CTX_load_verify_locations(ctx,"ca.crt",NULL);

	SSL_CTX_use_certificate_file(ctx,"server.crt",SSL_FILETYPE_PEM);

	SSL_CTX_use_PrivateKey_file(ctx,"server.key",SSL_FILETYPE_PEM);

	if(!SSL_CTX_check_private_key(ctx)){
		cerr<<"1\n";
		exit(4);
	}

	memset(&_sock,0,sizeof(_sock));
	serverfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	_sock.sin_family=AF_INET;
	_sock.sin_port=htons(6666);
	_sock.sin_addr.s_addr=htons(INADDR_ANY);
	
	bind(serverfd,(sockaddr*)&_sock,sizeof(_sock));
	listen(serverfd,10);
	while(1){
		tfd=accept(serverfd,(struct sockaddr*)NULL,NULL);
		SSL* ssl=SSL_new(ctx);
		char buffer[256];
		SSL_set_fd(ssl,tfd);
		SSL_read(ssl,buffer,sizeof(buffer));
		cout<<buffer<<endl;
		cin>>buffer;
		SSL_write(ssl,buffer,sizeof(buffer));
		close(tfd);
		SSL_free(ssl);
		SSL_CTX_free(ctx);		
	}
}
