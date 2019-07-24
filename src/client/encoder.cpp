//
// Created by a on 11/17/18.
//

#include "encoder.hpp"

encoder::encoder(powClient* powObjTemp)
{
    cryptoObj_ = new CryptoPrimitive();
    powObj_ = powObjTemp;
}

encoder::~encoder()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}

void encoder::run()
{
    while (true) {
        Chunk_t tempChunk;
        if (inputMQ_.done_ && !extractMQFromKeyClient(tempChunk)) {
            break;
        }
        encodeChunk(tempChunk);
        insertMQToPOW(tempChunk);
    }
    powObj_->editJobDoneFlag();
    pthread_exit(NULL);
}

bool encoder::encodeChunk(Chunk_t& newChunk)
{
    cryptoObj_->encryptChunk(newChunk);
    cryptoObj_->generateHash(newChunk.logicData, newChunk.logicDataSize, newChunk.chunkHash);
}

bool encoder::insertMQFromKeyClient(Chunk_t& newChunk)
{
    return inputMQ_.push(newChunk);
}

bool encoder::extractMQFromKeyClient(Chunk_t& newChunk)
{
    return inputMQ_.pop(newChunk);
}

bool encoder::insertMQToPOW(Chunk_t& newChunk)
{
    return powObj_->insertMQFromEncoder(newChunk);
}

bool encoder::editJobDoneFlag()
{
    inputMQ_.done_ = true;
    if (inputMQ_.done_) {
        return true;
    } else {
        return false;
    }
}