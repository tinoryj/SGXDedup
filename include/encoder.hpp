#ifndef GENERALDEDUPSYSTEM_ENCODER_HPP
#define GENERALDEDUPSYSTEM_ENCODER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "messageQueue.hpp"
#include "powClient.hpp"

extern Configure config;

class Encoder {
private:
    CryptoPrimitive* cryptoObj_;
    powClient* powObj_;
    messageQueue<Data_t> inputMQ_;

public:
    bool encodeChunk(Data_t& newChunk);
    Encoder(powClient* powObjTemp);
    ~Encoder();
    void run();
    bool insertMQFromKeyClient(Data_t& newChunk);
    bool extractMQFromKeyClient(Data_t& newChunk);
    bool insertMQToPOW(Data_t& newChunk);
    bool editJobDoneFlag();
};

#endif //GENERALDEDUPSYSTEM_ENCODER_HPP
