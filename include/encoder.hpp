#ifndef GENERALDEDUPSYSTEM_ENCODER_HPP
#define GENERALDEDUPSYSTEM_ENCODER_HPP

#include "chunker.hpp"
#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "keyClient.hpp"
#include "powClient.hpp"

extern Configure config;

class encoder {
private:
    CryptoPrimitive* cryptoObj_;
    powClient* powObj_;
    messageQueue<Chunk_t> inputMQ_;

public:
    bool encodeChunk(Chunk_t& newChunk);
    encoder(powClient* powObjTemp);
    ~encoder();
    void run();
    bool insertMQFromKeyClient(Chunk_t newChunk);
    bool extractMQFromKeyClient(Chunk_t newChunk);
    bool insertMQToPOW(Chunk_t newChunk);
    bool editJobDoneFlag();
};

#endif //GENERALDEDUPSYSTEM_ENCODER_HPP
