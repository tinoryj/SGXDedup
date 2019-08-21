#include "keyClient.hpp"
#include "openssl/rsa.h"
#include <sys/time.h>

extern Configure config;

struct timeval timestartKey;
struct timeval timeendKey;

void PRINT_BYTE_ARRAY_KEY_CLIENT(
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

keyClient::keyClient(powClient* powObjTemp, u_char* keyExchangeKey)
{
    inputMQ_ = new messageQueue<Data_t>(config.get_Data_t_MQSize());
    powObj_ = powObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, 16);
    socket_.init(CLIENT_TCP, config.getKeyServerIP(), config.getKeyServerPort());
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
    inputMQ_->~messageQueue();
    delete inputMQ_;
}

void keyClient::run()
{
    gettimeofday(&timestartKey, NULL);
    vector<Data_t> batchList;
    int batchNumber = 0;
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_];
    u_char chunkHash[CHUNK_HASH_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
    while (true) {

        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            cerr << "KeyClient : Chunker jobs done, queue is empty" << endl;
            JobDoneFlag = true;
        }
        if (extractMQFromChunker(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                insertMQToPOW(tempChunk);
                continue;
            }
            batchList.push_back(tempChunk);
            memcpy(chunkHash + batchNumber * CHUNK_HASH_SIZE, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            batchNumber++;
        }
        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            int batchedKeySize = 0;

            if (!keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize)) {
                cerr << "KeyClient : error get key for " << setbase(10) << batchNumber << " chunks" << endl;
                return;
            } else {
                for (int i = 0; i < batchNumber; i++) {
                    memcpy(batchList[i].chunk.encryptKey, chunkKey + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
                    // cerr << "KeyClient : chunk hash & key for chunk " << i << " = " << endl;
                    // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, chunkHash + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
                    // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, chunkKey + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
                    if (encodeChunk(batchList[i])) {
                        insertMQToPOW(batchList[i]);
                    } else {
                        cerr << "KeyClient : encode chunk error, exiting" << endl;
                        return;
                    }
                }
                batchList.clear();
                memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
                memset(chunkKey, 0, CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_);
                batchNumber = 0;
            }
        }
        if (JobDoneFlag) {
            if (!powObj_->editJobDoneFlag()) {
                cerr << "KeyClient : error to set job done flag for encoder" << endl;
            } else {
                cerr << "KeyClient : key exchange thread job done, set job done flag for encoder done, exit now" << endl;
            }
            break;
        }
    }

    gettimeofday(&timeendKey, NULL);
    long diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
    double second = diff / 1000000.0;
    printf("Key client thread work time is %ld us = %lf s\n", diff, second);
    return;
}

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    for (int i = 0; i < batchNumber; i++) {
        cryptoObj_->keyExchangeEncrypt(batchHashList + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash + i * CHUNK_ENCRYPT_KEY_SIZE);
    }
    if (!socket_.Send(sendHash, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "keyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!socket_.Recv(recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "keyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
        return false;
    }
    batchkeyNumber = recvSize / CHUNK_ENCRYPT_KEY_SIZE;
    if (batchkeyNumber == batchNumber) {
        for (int i = 0; i < batchkeyNumber; i++) {
            u_char key[CHUNK_ENCRYPT_KEY_SIZE];
            cryptoObj_->keyExchangeDecrypt(recvBuffer + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, key);
            // cerr << "KeyClient : encrypted data " << i << endl;
            // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, recvBuffer + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
            // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, sendHash + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
            // cerr << "KeyClient : decrypt data " << i << endl;
            // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, batchHashList + i * CHUNK_HASH_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
            // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, key, CHUNK_ENCRYPT_KEY_SIZE);
            memcpy(batchKeyList + i * CHUNK_ENCRYPT_KEY_SIZE, key, CHUNK_ENCRYPT_KEY_SIZE);
        }
        return true;
    } else {
        return false;
    }
}

bool keyClient::encodeChunk(Data_t& newChunk)
{
    bool statusChunk = cryptoObj_->encryptChunk(newChunk.chunk);
    bool statusHash = cryptoObj_->generateHash(newChunk.chunk.logicData, newChunk.chunk.logicDataSize, newChunk.chunk.chunkHash);
    if (!statusChunk) {
        cerr << "KeyClient : error encrypt chunk" << endl;
        return false;
    } else if (!statusHash) {
        cerr << "KeyClient : error compute hash" << endl;
        return false;
    } else {
        return true;
    }
}

bool keyClient::insertMQFromChunker(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool keyClient::extractMQFromChunker(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool keyClient::insertMQToPOW(Data_t& newChunk)
{
    return powObj_->insertMQFromEncoder(newChunk);
}

bool keyClient::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}
