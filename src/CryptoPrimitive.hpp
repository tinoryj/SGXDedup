#ifndef CRYPTOPRIMITIVE_HPP
#define CRYPTOPRIMITIVE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /*for uint32_t*/
#include <string.h>

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

class CryptoPrimitive{
	private:
		int _cryptoType;

		EVP__mdCTX _mdCTX;
		const EVP_MD *_md;

		EVP__cipherCTX _cipherctx;
		const EVP_CIPHER *_cipher;

		unsigned char _iv[16];

		int _hashSize;
		int keySize_;
		int blockSize_;

	public:
		CryptoPrimitive(int cryptoType = HIGH_SEC_PAIR_TYPE);
		~CryptoPrimitive();
		int getHashSize();
		int getKeySize();
		int getBlockSize();
		bool generateHash(unsigned char *dataBuffer, const int &dataSize, unsigned char *hash);
		bool encryptWithKey(unsigned char *dataBuffer, const int &dataSize, unsigned char *key, unsigned char *ciphertext);
		bool decryptWithKey(unsigned char *ciphertext, const int &dataSize, unsigned char *key, unsigned char *dataBuffer);
};


CryptoPrimitive::CryptoPrimitive(int cryptoType){
	_cryptoType = cryptoType;

	if (_cryptoType == HIGH_SEC_PAIR_TYPE) {
		EVP__mdCTX_init(&_mdCTX);
		_md = EVP_sha256();
		_hashSize = 32;
		EVP__cipherCTX_init(&_cipherctx);

		_cipher = EVP_aes_256_cbc();
		keySize_ = 32;
		blockSize_ = 16;

	}

	if (_cryptoType == LOW_SEC_PAIR_TYPE) {
		EVP__mdCTX_init(&_mdCTX);

		_md = EVP_md5();
		_hashSize = 16;

		EVP__cipherCTX_init(&_cipherctx);

		_cipher = EVP_aes_128_cbc();
		keySize_ = 16;
		blockSize_ = 16;

		_iv = (unsigned char *) malloc(sizeof(unsigned char) * blockSize_);
		memset(_iv, 0, blockSize_); 	
	}

	if (_cryptoType == SHA256_TYPE) {
		EVP__mdCTX_init(&_mdCTX);

		_md = EVP_sha256();
		_hashSize = 32;

		keySize_ = -1;
		blockSize_ = -1;
	}

	if (_cryptoType == SHA1_TYPE) {
		EVP__mdCTX_init(&_mdCTX);
		_md = EVP_sha1();
		_hashSize = 20;

		keySize_ = -1;
		blockSize_ = -1;

	}	
}
CryptoPrimitive::~CryptoPrimitive(){
	if ((_cryptoType == HIGH_SEC_PAIR_TYPE) || (_cryptoType == LOW_SEC_PAIR_TYPE)) {
		EVP__mdCTX_cleanup(&_mdCTX);

		EVP__cipherCTX_cleanup(&_cipherctx);
		free(_iv);	
	}

	if ((_cryptoType == SHA256_TYPE) || (_cryptoType == SHA1_TYPE)) {
		EVP__mdCTX_cleanup(&_mdCTX);
	}
}


bool CryptoPrimitive::generateHash(unsigned char *dataBuffer, const int &dataSize, unsigned char *hash) {
	int hashSize;

	EVP_DigestInit_ex(&_mdCTX, _md, NULL);
	EVP_DigestUpdate(&_mdCTX, dataBuffer, dataSize);
	EVP_DigestFinal_ex(&_mdCTX, hash, (unsigned int*) &hashSize);

	if (hashSize != _hashSize) {
		fprintf(stderr, "Error: the size of the generated hash (%d bytes) does not match with the expected one (%d bytes)!\n", 
				hashSize, _hashSize);

		return 0;
	}	

	return 1;
}

bool CryptoPrimitive::encryptWithKey(unsigned char *dataBuffer, const int &dataSize, unsigned char *key, 
		unsigned char *ciphertext) {
	int ciphertextSize, ciphertextTailSize;	

	if (dataSize % blockSize_ != 0) {
		fprintf(stderr, "Error: the size of the input data (%d bytes) is not a multiple of that of encryption block unit (%d bytes)!\n", 
				dataSize, blockSize_);

		return 0;
	}

	EVP_EncryptInit_ex(&_cipherctx, _cipher, NULL, key, _iv);		
	EVP__cipherCTX_set_padding(&_cipherctx, 0);
	EVP_EncryptUpdate(&_cipherctx, ciphertext, &ciphertextSize, dataBuffer, dataSize);
	EVP_EncryptFinal_ex(&_cipherctx, ciphertext + ciphertextSize, &ciphertextTailSize);
	ciphertextSize += ciphertextTailSize;

	if (ciphertextSize != dataSize) {
		fprintf(stderr, "Error: the size of the cipher output (%d bytes) does not match with that of the input (%d bytes)!\n", 
				ciphertextSize, dataSize);

		return 0;
	}	

	return 1;
}

bool CryptoPrimitive::decryptWithKey(unsigned char *ciphertext, const int &dataSize, unsigned char *key, 
		unsigned char *dataBuffer) {
	int plaintextSize, plaintextTailSize;	

	if (dataSize % blockSize_ != 0) {
		fprintf(stderr, "Error: the size of the input data (%d bytes) is not a multiple of that of encryption block unit (%d bytes)!\n", 
				dataSize, blockSize_);

		return 0;
	}

	EVP_DecryptInit_ex(&_cipherctx, _cipher, NULL, key, _iv);		
	EVP__cipherCTX_set_padding(&_cipherctx, 0);
	EVP_DecryptUpdate(&_cipherctx, dataBuffer, &plaintextSize, ciphertext, dataSize);
	EVP_DecryptFinal_ex(&_cipherctx, dataBuffer + plaintextSize, &plaintextTailSize);
	plaintextSize += plaintextTailSize;

	if (plaintextSize != dataSize) {
		fprintf(stderr, "Error: the size of the plaintext output (%d bytes) does not match with that of the original data (%d bytes)!\n", 
				plaintextSize, dataSize);

		return 0;
	}	

	return 1;
}


int CryptoPrimitive::getHashSize() {
	return _hashSize;
}

int CryptoPrimitive::getKeySize() {
	return keySize_;
}

int CryptoPrimitive::getBlockSize() {
	return blockSize_;
}

#endif