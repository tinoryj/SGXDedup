//
// Created by a on 1/30/19.
//

#ifndef GENERALDEDUPSYSTEM_DECODER_HPP
#define GENERALDEDUPSYSTEM_DECODER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "protocol.hpp"

class decoder {
private:
    CryptoPrimitive* crypto;
    map<string, string> _keyRecipe;
    bool getKey(Chunk_t& newChunk);
    bool decodeChunk(Chunk_t& newChunk);
    void runDecode();
    bool outputDecodeRecoder();

public:
    void run();
    decoder();
    ~decoder();
};

#endif //GENERALDEDUPSYSTEM_DECODER_HPP
