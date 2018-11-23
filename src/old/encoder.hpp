#ifndef ENCODE_HPP
#define ENCODE_HPP

#include "_encoder.hpp"
#include "CryptoPrimitive.hpp"
#include "chunke.hpp"

extern Configure config;
extern keyManger keyobj;

class encoder:public _Encoder{
private:
	CryptoPrimitive *_cryptoObj
	
public:
	encoder();
	~encoder();
	void run();
};

encoder::encoder(){};

encoder::~encoder(){};


#endif