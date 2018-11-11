#include "_CryptoPrimitive.hpp"

_CryptoPrimitive::_CryptoPrimitive(int cryptoType){}

_CryptoPrimitive::~_CryptoPrimitive(){
	if ((_cryptoType == HIGH_SEC_PAIR_TYPE) || (_cryptoType == LOW_SEC_PAIR_TYPE)) {
		EVP__mdCTX_cleanup(&_mdCTX);

		EVP__cipherCTX_cleanup(&_cipherctx);
		free(_iv);	
	}

	if ((_cryptoType == SHA256_TYPE) || (_cryptoType == SHA1_TYPE)) {
		EVP__mdCTX_cleanup(&_mdCTX);
	}
}

int _CryptoPrimitive::getHashSize(){
	return _hashSize;
}

int _CryptoPrimitive::getKeySize(){
	return _keySize
}

int _CryptoPrimitive::getBlockSize(){
	return _blockSize;
}