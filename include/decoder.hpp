//
// Created by a on 1/30/19.
//

#ifndef GENERALDEDUPSYSTEM_DECODER_HPP
#define GENERALDEDUPSYSTEM_DECODER_HPP

#include "_decoder.hpp"
#include "CryptoPrimitive.hpp"
#include "seriazation.hpp"
#include "protocol.hpp"


class decoder:public _Decoder{
private:
    CryptoPrimitive *_crypto;
    map<string,string>_keyRecipe;
    bool getKey(Chunk &newChunk);
    bool decodeChunk(Chunk &newChunk);
    void runDecode();
    bool outputDecodeRecoder();

public:
    void run();
    decoder();
    ~decoder();
};


#endif //GENERALDEDUPSYSTEM_DECODER_HPP
