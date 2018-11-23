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

#define CLCERT "key/client.crt"
#define CLKEY  "key/client.key"
#define CACERT "key/cacert.crt"
#define PORT 6666
#define IP "127.0.0.1"

int main(){
	int clientfd;
	struct sockaddr_in _sock;
	SSL* ssl;
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

	SSL_CTX *ctx=SSL_CTX_new(TLSv1_client_method());
	SSL_load_error_strings();
	SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);	
	SSL_CTX_load_verify_locations(ctx,CACERT,NULL);
	SSL_CTX_use_certificate_file(ctx,CLCERT,SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ctx,CLKEY,SSL_FILETYPE_PEM);

	if(!SSL_CTX_check_private_key(ctx)){
		cerr<<"1\n";
		exit(4);
	}	

	memset(&_sock,0,sizeof(_sock));
	clientfd=socket(AF_INET,SOCK_STREAM,0);
	_sock.sin_family=AF_INET;
	_sock.sin_addr.s_addr=inet_addr(IP);
	_sock.sin_port=htons(PORT);
	
	connect(clientfd,(struct sockaddr*)&_sock,sizeof(_sock));

	ssl=SSL_new(ctx);
	SSL_set_fd(ssl,clientfd);
	SSL_connect(ssl);
	char buffer[256];
	cin>>buffer;
	SSL_write(ssl,buffer,sizeof(buffer));
	SSL_read(ssl,buffer,sizeof(buffer));
	cout<<buffer;
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}