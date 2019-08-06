#include "keyClient.hpp"
#include "openssl/rsa.h"
#include <sys/time.h>

extern Configure config;
extern KeyCache kCache;

struct timeval timestartKey;
struct timeval timeendKey;

keyClient::keyClient(Encoder* encoderObjTemp)
{
    inputMQ_ = new messageQueue<Data_t>(3000);
    encoderObj_ = encoderObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    socket_.init(CLIENT_TCP, config.getKeyServerIP(), config.getKeyServerPort());
    kmServer server(socket_);
    powSession* session = server.authkm();
    if (session != nullptr) {
        trustdKM_ = true;
        memcpy(keyExchangeKey_, session->smk, 16);
        delete session;
    } else {
        trustdKM_ = false;
        delete session;
        cerr << "KeyClient : keyServer enclave not trusted, client exit now" << endl;
        exit(0);
    }
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}
void PRINT_BYTE_ARRAY(
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
void keyClient::run()
{
    gettimeofday(&timestartKey, NULL);
    vector<Data_t> batchList;
    batchList.resize(keyBatchSize_);
    int batchNumber = 0;
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_];
    u_char chunkHash[CHUNK_HASH_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
    while (true) {

        Data_t tempchunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            cerr << "KeyClient : Chunker jobs done, queue is empty" << endl;
            JobDoneFlag = true;
        }
        if (extractMQFromChunker(tempchunk)) {
            if (tempchunk.dataType == DATA_TYPE_RECIPE) {
                cerr << "KeyClient : get file recipe head frome message queue" << endl;
                continue;
            }
            string tempHashForCache;
            tempHashForCache.resize(CHUNK_HASH_SIZE);
            memcpy(&tempHashForCache[0], tempchunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            if (kCache.existsKeyinCache(tempHashForCache)) {
                cerr << "KeyClient : chunkHash hit kCache, hash = " << endl;
                PRINT_BYTE_ARRAY(stderr, tempchunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                string hitCacheTemp = kCache.getKeyFromCache(tempHashForCache);
                memcpy(tempchunk.chunk.encryptKey, &hitCacheTemp[0], CHUNK_ENCRYPT_KEY_SIZE);
                //insertMQtoEncoder(tempchunk);
            } else {
                batchList.push_back(tempchunk);
                memcpy(chunkHash + batchNumber * CHUNK_HASH_SIZE, tempchunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                batchNumber++;
            }
        }
        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            int batchKeySize = 0;

            if (!keyExchange(chunkHash, batchNumber, chunkKey, batchKeySize)) {
                cerr << "KeyClient : error get key for " << setbase(10) << batchNumber << " chunks" << endl;
                batchList.clear();
                memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
                memset(chunkKey, 0, CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_);
                batchNumber = 0;
            } else {
                cerr << "KeyClient : key exchange for " << setbase(10) << batchNumber << " chunks over" << endl;
                for (int i = 0; i < batchNumber; i++) {
                    memcpy(batchList[i].chunk.encryptKey, chunkKey + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
                    string tempHash((char*)batchList[i].chunk.chunkHash, CHUNK_HASH_SIZE);
                    string tempKey((char*)batchList[i].chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                    kCache.insertKeyToCache(tempHash, tempKey);
                    //insertMQtoEncoder(batchList[i]);
                }
                batchList.clear();
                memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
                memset(chunkKey, 0, CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_);
                batchNumber = 0;
            }
        }
        if (JobDoneFlag) {
            break;
        } else {
            continue;
        }
    }
    cerr << "KeyClient : key exchange thread job done, exit now" << endl;

    gettimeofday(&timeendKey, NULL);
    long diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
    double second = diff / 1000000.0;
    printf("Key client thread work time is %ld us = %lf s\n", diff, second);

    exit(0);
    pthread_exit(0);
    //close ssl connection
}

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    cryptoObj_->keyExchangeEncrypt(batchHashList, CHUNK_HASH_SIZE * batchNumber, keyExchangeKey_, keyExchangeKey_, sendHash);
    if (!socket_.Send(sendHash, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "keyClient: send socket error" << endl;
        exit(0);
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!socket_.Recv(recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        exit(0);
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "keyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
        exit(0);
    }
    cryptoObj_->keyExchangeDecrypt(recvBuffer, recvSize, keyExchangeKey_, keyExchangeKey_, batchKeyList);
    batchkeyNumber = recvSize / CHUNK_ENCRYPT_KEY_SIZE;
    if (batchkeyNumber == batchNumber) {
        return true;
    } else {
        return false;
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

bool keyClient::insertMQtoEncoder(Data_t& newChunk)
{
    return encoderObj_->insertMQFromKeyClient(newChunk);
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