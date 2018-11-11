#ifndef CRYPTOPRIMITIVE_HPP
#define CRYPTOPRIMITIVE_HPP
#include "_CryptoPrimitive.hpp"

class CryptoPrimitive:public _CryptoPrimitive{
		string generateHash(std::string data);
		bool encryptWithKey(unsigned char *dataBuffer, const int &dataSize, unsigned char *key, unsigned char *ciphertext);
		bool decryptWithKey(unsigned char *ciphertext, const int &dataSize, unsigned char *key, unsigned char *dataBuffer);
};

#endif