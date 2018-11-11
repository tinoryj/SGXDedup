#ifndef KETMANGER_HPP
#define KETMANGER_HPP

#include "_keyManager.hpp"
#include "chunk.hpp"
#include "ssl.hpp"
#include <string>
#include "configure.hpp"

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "./key/private.pem"

class keyServer:public _keyManager{
private:
	ssl* _keySecurityChannel;
	RSA* _rsa;
	BIO* _key;
	BN_CTX* _bnCTX;

public:
	keyServer();
	~keyServer();
	void run();
	void threadHandle(SSL* sslConnection);
	bool keyGen(std::string hash,std::string& key);
};

#endif