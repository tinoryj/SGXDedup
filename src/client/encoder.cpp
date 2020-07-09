#include "encoder.hpp"
#include "openssl/rsa.h"
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
    inputMQ_ = new messageQueue<Data_t>;
    powObj_ = powObjTemp;
    cryptoObj_ = new CryptoPrimitive();
}

Encoder::~Encoder()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
    inputMQ_->~messageQueue();
    delete inputMQ_;
}

void Encoder::run()
{

#if SYSTEM_BREAK_DOWN == 1
    double encryptionTime = 0;
    long diff;
    double second;
#endif
    bool JobDoneFlag = false;
    while (true) {

        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            cerr << "Encoder : Key Client jobs done, queue is empty" << endl;
            JobDoneFlag = true;
        }
        if (extractMQFromKeyClient(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                insertMQToPOW(tempChunk);
                continue;
            }

#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartEncoder, NULL);
#endif
            bool encodeChunkStatus = encodeChunk(tempChunk);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendEncoder, NULL);
            diff = 1000000 * (timeendEncoder.tv_sec - timestartEncoder.tv_sec) + timeendEncoder.tv_usec - timestartEncoder.tv_usec;
            second = diff / 1000000.0;
            encryptionTime += second;
#endif
            if (encodeChunkStatus) {
                insertMQToPOW(tempChunk);
            } else {
                cerr << "Encoder : encode chunk error, exiting" << endl;
                return;
            }
        }
        if (JobDoneFlag) {
            if (!powObj_->editJobDoneFlag()) {
                cerr << "Encoder : error to set job done flag for encoder" << endl;
            } else {
                cerr << "Encoder : encode chunk thread job done, set job done flag for encoder done, exit now" << endl;
            }
            break;
        }
    }
#if SYSTEM_BREAK_DOWN == 1
    cout << "Encoder : chunk encryption work time = " << encryptionTime << " s" << endl;
#endif
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
