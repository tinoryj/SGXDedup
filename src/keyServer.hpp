#ifndef KETMANGER_HPP
#define KETMANGER_HPP

#include "_keyManager.hpp"
#include "chunk.hpp"
#include "ssl.hpp"
#include <string>
#include <boost/lockfree/queue.hpp>

#define SERVERSIDE 0
#define CLIENTSIDE 1

extern configure config;

class keyServer::public _KeyManager{
private:
	ssl* _keyConnection;
	RSA* _rsa;
	BIO* _key;
	BN_CTX* _bnCTX;

public:

};

bool keyServer::keyGen(string hash,string& key){
	BIGNUM* result;
	char buffer[128];
	memset(buffer,0,sizeof(buffer));

	BN_bin2bn(hash.c_str(),128,result);

	//result=hash^d
	BN_mod_exp(result,result,_rsa->d,_rsa->n,_bnCTX);
	BN_bn2bin(result,buffer+(128-BN_num_bytes(result)),)
	key=buffer;
}
#endif