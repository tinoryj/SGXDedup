#ifndef _CRYPTOPRIMITIVE
#define _CRYPTOPRIMITIVE

#include <string>

/*for the use of OpenSSL*/
#include <openssl/evp.h>
#include <openssl/crypto.h>

//#define OPENSSL_THREAD_DEFINES

#include <openssl/opensslconf.h>
/*macro for OpenSSL debug*/
#define OPENSSL_DEBUG 0
/*for the use of mutex lock*/
//#include <pthread.h>

/*macro for the type of a high secure pair of hash generation and encryption*/
#define HIGH_SEC_PAIR_TYPE 0
/*macro for the type of a low secure pair of hash generation and encryption*/
#define LOW_SEC_PAIR_TYPE 1
/*macro for the type of a SHA-256 hash generation*/
#define SHA256_TYPE 2
/*macro for the type of a SHA-1 hash generation*/
#define SHA1_TYPE 3

using namespace std;

class _CryptoPrimitive{
private:

		int _cryptoType;

		EVP_MD_CTX _mdCTX;
		const EVP_MD *_md;

		EVP_CIPHER_CTX _cipherctx;
		const EVP_CIPHER *_cipher;

		unsigned char _iv[16];

		int _hashSize;
		int _keySize;
		int _blockSize;

public:
	_CryptoPrimitive(int cryptoType);
	~_CryptoPrimitive();
	int getHashSize();
	int getKeySize();
	int getBlockSize();

	virtual string generaHash(std::string data);
	virtual bool encryptWithKey(std::string data,std::string key,std::string& cipherText)=0;
	virtual bool decryptWithKey(std::string data,std::string key,std::string& plaintText)=0;
};



#endif