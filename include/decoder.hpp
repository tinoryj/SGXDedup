#ifndef GENERALDEDUPSYSTEM_DECODER_HPP
#define GENERALDEDUPSYSTEM_DECODER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>

using namespace std;

class decoder {
private:
    CryptoPrimitive* cryptoObj_;
    KeyRecipeList_t keyRecipeList_;
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
