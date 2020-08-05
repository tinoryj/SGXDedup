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
    long diff;
    double second;
    double encryptChunkContentTime = 0;
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_ONLY_KEY_RECIPE_FILE
    double encryptChunkKeyTime = 0;
#endif //RECIPE_MANAGEMENT_METHOD
#endif // SYSTEM_BREAK_DOWN
    bool JobDoneFlag = false;
    while (true) {

        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            JobDoneFlag = true;
        }
        if (extractMQ(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                powObj_->insertMQ(tempChunk);
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
                    }
#endif
                    powObj_->insertMQ(tempChunk);
                }
            }
        }
        if (JobDoneFlag) {
            if (!powObj_->editJobDoneFlag()) {
                cerr << "Encoder : error to set job done flag for encoder" << endl;
            }
            break;
        }
    }
#if SYSTEM_BREAK_DOWN == 1
    cout << "Encoder : chunk content encryption work time = " << encryptChunkContentTime << " s" << endl;
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_ONLY_KEY_RECIPE_FILE
    cout << "Encoder : chunk key encryption work time = " << encryptChunkKeyTime << " s" << endl;
#endif
#endif
    return;
}

bool Encoder::insertMQ(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool Encoder::extractMQ(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
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
