#include "encoder.hpp"
#include <sys/time.h>

extern Configure config;

struct timeval timestartEncoder;
struct timeval timeendEncoder;

void PRINT_BYTE_ARRAY_ENCODER(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

Encoder::Encoder(powClient* powObjTemp)
{
    inputMQ_ = new messageQueue<Data_t>(3000);
    cryptoObj_ = new CryptoPrimitive();
    powObj_ = powObjTemp;
}

Encoder::~Encoder()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
    delete inputMQ_;
}

void Encoder::run()
{
    gettimeofday(&timestartEncoder, NULL);
    int count = 0;
    while (true) {
        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            break;
        }
        if (extractMQFromKeyClient(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                cerr << "Encoder : get file recipe head frome message queue, file size = " << tempChunk.recipe.fileRecipeHead.fileSize << " file chunk number = " << tempChunk.recipe.fileRecipeHead.totalChunkNumber << endl;
                PRINT_BYTE_ARRAY_ENCODER(stderr, tempChunk.recipe.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
                insertMQToPOW(tempChunk);
                continue;
            } else if (tempChunk.dataType == DATA_TYPE_CHUNK) {
                // cerr << "Encoder : current chunk ID = " << tempChunk.chunk.ID << " chunk data size = " << tempChunk.chunk.logicDataSize << endl;
                // cerr << setw(6) << "chunk hash = " << endl;
                // PRINT_BYTE_ARRAY_ENCODER(stderr, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                // cerr << setw(6) << "chunk key = " << endl;
                // PRINT_BYTE_ARRAY_ENCODER(stderr, tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                encodeChunk(tempChunk);
                count++;
                insertMQToPOW(tempChunk);
            }
        }
    }
    powObj_->editJobDoneFlag();
    cerr << "Encoder : encode " << count << " chunks over, thread exit" << endl;
    gettimeofday(&timeendEncoder, NULL);
    long diff = 1000000 * (timeendEncoder.tv_sec - timestartEncoder.tv_sec) + timeendEncoder.tv_usec - timestartEncoder.tv_usec;
    double second = diff / 1000000.0;
    printf("Encoder : thread work time is %ld us = %lf s\n", diff, second);
    return;
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
    return inputMQ_->push(newChunk);
}

bool Encoder::extractMQFromKeyClient(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool Encoder::insertMQToPOW(Data_t& newChunk)
{
    return powObj_->insertMQFromEncoder(newChunk);
}

bool Encoder::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}