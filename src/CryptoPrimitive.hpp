#ifndef CRYPTOPRIMITIVE_HPP
#define CRYPTOPRIMITIVE_HPP
#include "_cryptoPrimitive.hpp"

class cryptoPrimitive:public _cryptoPrimitive{
		string generateHash(std::string data);
		bool encryptWithKey(unsigned char *dataBuffer, const int &dataSize, unsigned char *key, unsigned char *ciphertext);
		bool decryptWithKey(unsigned char *ciphertext, const int &dataSize, unsigned char *key, unsigned char *dataBuffer);
};

#endif