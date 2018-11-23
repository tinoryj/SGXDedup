#include "_cryptoPrimitive.hpp"

_cryptoPrimitive::_cryptoPrimitive(int cryptoType){}

_cryptoPrimitive::~_cryptoPrimitive(){
	if ((_cryptoType == HIGH_SEC_PAIR_TYPE) || (_cryptoType == LOW_SEC_PAIR_TYPE)) {
		EVP_MD_CTX_cleanup(&_mdCTX);

		EVP_CIPHER_CTX_cleanup(&_cipherctx);
		free(_iv);	
	}

	if ((_cryptoType == SHA256_TYPE) || (_cryptoType == SHA1_TYPE)) {
		EVP_MD_CTX_cleanup(&_mdCTX);
	}
}

int _cryptoPrimitive::getHashSize(){
	return _hashSize;
}

int _cryptoPrimitive::getKeySize(){
	return _keySize;
}

int _cryptoPrimitive::getBlockSize(){
	return _blockSize;
}