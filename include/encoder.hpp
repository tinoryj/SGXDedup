//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_ENCODER_HPP
#define GENERALDEDUPSYSTEM_ENCODER_HPP

#include "_encoder.hpp"
#include "CryptoPrimitive.hpp"
#include "chunker.hpp"
#include "_keyManager.hpp"

extern Configure config;
extern _keyManager *keyobj;

class encoder:public _Encoder{
private:
    CryptoPrimitive *_cryptoObj;

public:
    bool encodeChunk(Chunk& tmpChunk);
    encoder();
    ~encoder();
    void run();
};


#endif //GENERALDEDUPSYSTEM_ENCODER_HPP
