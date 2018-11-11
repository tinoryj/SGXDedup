#ifndef CRYPTOPRIMITIVE_HPP
#define CRYPTOPRIMITIVE_HPP


class CryptoPrimitive:public _CryptoPrimitive(){
		bool generateHash(unsigned char *dataBuffer, const int &dataSize, unsigned char *hash);
		bool encryptWithKey(unsigned char *dataBuffer, const int &dataSize, unsigned char *key, unsigned char *ciphertext);
		bool decryptWithKey(unsigned char *ciphertext, const int &dataSize, unsigned char *key, unsigned char *dataBuffer);
};

#endif