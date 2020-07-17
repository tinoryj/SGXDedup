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
#if QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_SPSC_QUEUE || QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_QUEUE
    inputMQ_->~messageQueue();
    delete inputMQ_;
#endif
}

void Encoder::run()
{

#if SYSTEM_BREAK_DOWN == 1
    double encryptChunkContentTime = 0;
    double encryptChunkKeyTime = 0;
    double generateCipherChunkHashTime = 0;
    long diff;
    double second;
#endif
    bool JobDoneFlag = false;
    while (true) {

        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            JobDoneFlag = true;
        }
        if (extractMQFromKeyClient(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                insertMQToPOW(tempChunk);
                continue;
            } else {
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartEncoder, NULL);
#endif
                u_char ciphertext[tempChunk.chunk.logicDataSize];
                bool encryptChunkContentStatus = cryptoObj_->encryptWithKey(tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize, tempChunk.chunk.encryptKey, ciphertext);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendEncoder, NULL);
                diff = 1000000 * (timeendEncoder.tv_sec - timestartEncoder.tv_sec) + timeendEncoder.tv_usec - timestartEncoder.tv_usec;
                second = diff / 1000000.0;
                encryptChunkContentTime += second;
#endif
                if (!encryptChunkContentStatus) {
                    cerr << "Encoder : cryptoPrimitive error, encrypt chunk logic data error" << endl;
                    return;
                } else {
                    memcpy(tempChunk.chunk.logicData, ciphertext, tempChunk.chunk.logicDataSize);

#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartEncoder, NULL);
#endif
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_ONLY_KEY_RECIPE_FILE
                    u_char cipherKey[CHUNK_ENCRYPT_KEY_SIZE];
                    bool encryptChunkKeyStatus = cryptoObj_->encryptWithKey(tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE, cryptoObj_->chunkKeyEncryptionKey_, cipherKey);
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendEncoder, NULL);
                    diff = 1000000 * (timeendEncoder.tv_sec - timestartEncoder.tv_sec) + timeendEncoder.tv_usec - timestartEncoder.tv_usec;
                    second = diff / 1000000.0;
                    encryptChunkKeyTime += second;
#endif
                    if (!encryptChunkKeyStatus) {
                        cerr << "Encoder : cryptoPrimitive error, encrypt chunk key error" << endl;
                        return;
                    } else {
                        memcpy(tempChunk.chunk.encryptKey, cipherKey, CHUNK_ENCRYPT_KEY_SIZE);
#endif
#if SYSTEM_BREAK_DOWN == 1
                        gettimeofday(&timestartEncoder, NULL);
#endif
                        bool generateCipherChunkHashStatus = cryptoObj_->generateHash(tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize, tempChunk.chunk.chunkHash);
#if SYSTEM_BREAK_DOWN == 1
                        gettimeofday(&timeendEncoder, NULL);
                        diff = 1000000 * (timeendEncoder.tv_sec - timestartEncoder.tv_sec) + timeendEncoder.tv_usec - timestartEncoder.tv_usec;
                        second = diff / 1000000.0;
                        generateCipherChunkHashTime += second;
#endif
                        if (generateCipherChunkHashStatus) {
                            insertMQToPOW(tempChunk);
                        } else {
                            cerr << "Encoder : generate cipher chunk hash error, exiting" << endl;
                            return;
                        }
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_ONLY_KEY_RECIPE_FILE
                    }
#endif
                }
            }
        }
        if (JobDoneFlag) {
            if (!powObj_->editJobDoneFlag()) {
                cerr << "Encoder : error to set job done flag for encoder" << endl;
            } else {
#if SYSTEM_BREAK_DOWN == 1
                cerr << "Encoder : encode chunk thread job done, exit now" << endl;
#endif
            }
            break;
        }
    }
#if SYSTEM_BREAK_DOWN == 1
    cout << "Encoder : chunk content encryption work time = " << encryptChunkContentTime << " s" << endl;
    cout << "Encoder : chunk key encryption work time = " << encryptChunkKeyTime << " s" << endl;
    cout << "Encoder : cipher chunk crypto hash generate work time = " << generateCipherChunkHashTime << " s" << endl;
#endif
    return;
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
