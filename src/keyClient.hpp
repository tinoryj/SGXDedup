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
	int _keyBatchSizeMin,_keyBatchSizeMax;


	RSA* _rsa;
	BIO* _key;
	BN_CTX *_bnCTX;
	BIGNUM* _r,invr;
	BIGNUM* _h;

public:
	keyClient();
	~keyClient();
	void run();
	string keyExchange(SSL* connection,Chunk champion);
	string elimination(string hash);
	string decoration(string key);
}


#endif