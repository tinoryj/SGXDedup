//
// Created by a on 11/17/18.
//

#include "encoder.hpp"

encoder::encoder(powClient* powObjTemp)
{
    cryptoObj = new CryptoPrimitive();
    powObj = powObjTemp;
}

encoder::~encoder()
{
    if (cryptoObj != NULL) {
        delete cryptoObj;
    }
}

void encoder::run()
{

    while (true) {
        Chunk_t tempChunk;
        if (inputMQ.done_ && !extractMQFromKeyClient(tempChunk)) {
            break;
        }
        encodeChunk(tempChunk);
        string data(tempChunk.logicData, tempChunk.logicDataSize), hash;
        cryptoObj->sha256_digest(data, hash);
        memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
        insertMQToPOW(tempChunk);
    }
    powObj->editJobDoneFlag();
    pthread_exit(NULL);
}

bool encoder::encodeChunk(Chunk_t& newChunk)
{
    cryptoObj->chunk_encrypt(newChunk);
}

bool encoder::insertMQFromKeyClient(Chunk_t newChunk)
{
    return inputMQ.push(newChunk);
}

bool encoder::extractMQFromKeyClient(Chunk_t newChunk)
{
    return inputMQ.pop(newChunk);
}

bool encoder::insertMQToPOW(Chunk_t newChunk)
{
    return powObj->insertMQFromEncoder(newChunk);
}

bool encoder::editJobDoneFlag()
{
    inputMQ.done_ = true;
}