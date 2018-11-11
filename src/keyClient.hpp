#ifndef KEYCLIENT_HPP
#define KEYCLIENT_HPP

#include "ssl.hpp"
#include "_messageQueue.hpp"
#include "configure.hpp"
#include "seriazation.hpp"
#include "chunk.hpp"

#define KEYMANGER_PUBLIC_KEY_FILE "/key/public.pem"

class keyClient{
private:
	_messageQueue _inputMQ;
	_messageQueue _outputMQ;
	ssl* _keySecurityChannel;
	int _minhashBatchSize;


	RSA* _rsa;
	BIO* _key;
	BN_CTX *_bnCTX;
	BIGNUM* _r,_invr;
	BIGNUM* _h;

public:
	keyClient();
	~keyClient();
	void run();
	void runForReceiveKey();
	string kex(SSL* connection,Chunk champion);
	string elimination(string hash);
	string decoration(string key);
}

string keyClient::elimination(string hash){

}

string keyClient::decoration(string hash){

}

#endif