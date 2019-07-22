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
        string data, hash;
        data.resize(tempChunk.logicDataSize);
        memcpy(&data[0], tempChunk.logicData, tempChunk.logicDataSize);
        cryptoObj_->sha256_digest(data, hash);
        memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
        insertMQToPOW(tempChunk);
    }
    powObj_->editJobDoneFlag();
    pthread_exit(NULL);
}

bool encoder::encodeChunk(Chunk_t& newChunk)
{
    cryptoObj_->chunk_encrypt(newChunk);
}

bool encoder::insertMQFromKeyClient(Chunk_t newChunk)
{
    return inputMQ_.push(newChunk);
}

bool encoder::extractMQFromKeyClient(Chunk_t newChunk)
{
    return inputMQ_.pop(newChunk);
}

bool encoder::insertMQToPOW(Chunk_t newChunk)
{
    return powObj_->insertMQFromEncoder(newChunk);
}

bool encoder::editJobDoneFlag()
{
    inputMQ_.done_ = true;
}