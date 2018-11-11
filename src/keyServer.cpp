#include "keyServer.hpp"

extern Configure config;

keyServer::keyServer(){
	_keySecurityChannel=new ssl(config.getKeyServerIP(),config.getKeyServerPort(),SERVERSIDE);
	_rsa=RSA_new();
	_key=BIO_new_file(KEYMANGER_PRIVATE_KEY,"w");
	PEM_read_bio_RSAPrivateKey(_key,&_rsa,NULL,NULL);

	_bnCTX=BN_CTX_new();
}

keyServer::~keyServer(){
	BIO_free_all(_key);
	RSA_free(_rsa);
}

void keyServer::run(){
	SSL* sslConnection=_keySecurityChannel->sslListen();
	threadHandle(sslConnection);                                                                                              
}
void keyServer::threadHandle(SSL* sslConnection){
	std::string key,hashbuffer;
	while(1){
		_keySecurityChannel->sslRead(sslConnection,hashbuffer);
		this->workloadProgress(hashbuffer,key);
		_keySecurityChannel->sslWrite(sslConnection,key.c_str());
	}
}

bool keyServer::keyGen(std::string hash,std::string& key){
	BIGNUM* result;
	char buffer[128];
	memset(buffer,0,sizeof(buffer));

	BN_bin2bn((const unsigned char*)hash.c_str(),128,result);

	//result=hash^d
	BN_mod_exp(result,result,_rsa->d,_rsa->n,_bnCTX);
	BN_bn2bin(result,(unsigned char*)buffer+(128-BN_num_bytes(result)));
	key=buffer;
}