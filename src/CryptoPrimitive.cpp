#include "CryptoPrimitive.hpp"

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



bool CryptoPrimitive::generateHash(string data,string hash){
	int hashSize;

	EVP_DigestInit_ex(&_mdCTX, _md, NULL);
	EVP_DigestUpdate(&_mdCTX, data, data.length());
	EVP_DigestFinal_ex(&_mdCTX, hash, (unsigned int*) &hashSize);

	if (hashSize != _hashSize) {
		fprintf(stderr, "Error: the size of the generated hash (%d bytes) does not match with the expected one (%d bytes)!\n", 
				hashSize, _hashSize);

		return 0;
	}	

	return 1;
}


bool CryptoPrimitive::encryptWithKey(string data,string key,string& cipherText) {
	int cipherTextSize, cipherTextTailSize;	
	
	/**********************************/
	char* buffer=new char[4096];
	/**********************************/

	if (data.length() % _blockSize != 0) {
		fprintf(stderr, "Error: the size of the input data (%d bytes) is not a multiple of that of encryption block unit (%d bytes)!\n", 
				data.length,_blockSize);

		return 0;
	}

	EVP_EncryptInit_ex(&_cipherctx, _cipher, NULL, key.c_str(), _iv);		
	EVP_cipherCTX_set_padding(&_cipherctx, 0);
	EVP_EncryptUpdate(&_cipherctx, buffer, &ciphertextSize, data.c_str(), dataSize);
	EVP_EncryptFinal_ex(&_cipherctx, buffer + ciphertextSize, &ciphertextTailSize);
	ciphertextSize += ciphertextTailSize;



	if (ciphertextSize != dataSize) {
		fprintf(stderr, "Error: the size of the cipher output (%d bytes) does not match with that of the input (%d bytes)!\n", 
				ciphertextSize, dataSize);

		return 0;
	}	

	cipherText=buffer;

	return 1;
}

bool CryptoPrimitive::decryptWithKey(string data,string key,string &plaintText){
	int plaintTextSize, plaintTextTailSize;	

	/******************************/
	char *buffer=new char[4096];
	/******************************/

	if (data.length() % blockSize_ != 0) {
		fprintf(stderr, "Error: the size of the input data (%d bytes) is not a multiple of that of encryption block unit (%d bytes)!\n", 
				data.length(), blockSize_);

		return 0;
	}

	EVP_DecryptInit_ex(&_cipherctx, _cipher, NULL, key, _iv);		
	EVP__cipherCTX_set_padding(&_cipherctx, 0);
	EVP_DecryptUpdate(&_cipherctx, buffer, &plaintTextSize, ciphertext.c_str(), data.length());
	EVP_DecryptFinal_ex(&_cipherctx, buffer + plaintTextSize, &plaintTextTailSize);
	plaintTextSize += plaintTextTailSize;

	if (plaintTextSize != data.length()) {
		fprintf(stderr, "Error: the size of the plaintText output (%d bytes) does not match with that of the original data (%d bytes)!\n", 
				plaintTextSize, data.length());

		return 0;
	}	

	plaintText=buffer;
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
