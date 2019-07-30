#include "encoder.hpp"

extern Configure config;

Encoder::Encoder(powClient* powObjTemp)
{
    cryptoObj_ = new CryptoPrimitive();
    powObj_ = powObjTemp;
}

Encoder::~Encoder()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}

void Encoder::run()
{
    while (true) {
        Data_t tempChunk;
        if (inputMQ_.done_ && !extractMQFromKeyClient(tempChunk)) {
            break;
        }
        if (tempChunk.dataType == DATA_TYPE_RECIPE) {
            continue;
        }
        encodeChunk(tempChunk);
        insertMQToPOW(tempChunk);
    }
    powObj_->editJobDoneFlag();
    pthread_exit(NULL);
}

bool Encoder::encodeChunk(Data_t& newChunk)
{
    bool statusChunk = cryptoObj_->encryptChunk(newChunk.chunk);
    bool statusHash = cryptoObj_->generateHash(newChunk.chunk.logicData, newChunk.chunk.logicDataSize, newChunk.chunk.chunkHash);
    if (!statusChunk) {
        cerr << "Encoder : error encrypt chunk" << endl;
        return false;
    } else if (!statusHash) {
        cerr << "Encoder : error compute hash" << endl;
        return false;
    } else {
        return true;
    }
}

bool Encoder::insertMQFromKeyClient(Data_t& newChunk)
{
    return inputMQ_.push(newChunk);
}

bool Encoder::extractMQFromKeyClient(Data_t& newChunk)
{
    return inputMQ_.pop(newChunk);
}

bool Encoder::insertMQToPOW(Data_t& newChunk)
{
    return powObj_->insertMQFromEncoder(newChunk);
}

bool Encoder::editJobDoneFlag()
{
    inputMQ_.done_ = true;
    if (inputMQ_.done_) {
        return true;
    } else {
        return false;
    }
}