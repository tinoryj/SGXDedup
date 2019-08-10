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

keyClient::keyClient(powClient* powObjTemp)
{
    inputMQ_ = new messageQueue<Data_t>(config.get_Data_t_MQSize());
    powObj_ = powObjTemp;
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
    delete inputMQ_;
}

void keyClient::run()
{
    gettimeofday(&timestartKey, NULL);
    vector<Data_t> batchList;
    //batchList.resize(keyBatchSize_);
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
                // cerr << "KeyClient : get file recipe head frome message queue, file size = " << tempChunk.recipe.fileRecipeHead.fileSize << " file chunk number = " << tempChunk.recipe.fileRecipeHead.totalChunkNumber << endl;
                // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, tempChunk.recipe.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
                insertMQToPOW(tempChunk);
                continue;
            }
            // cerr << "KeyClient : current extract  chunk ID = " << tempChunk.chunk.ID << " chunk data size = " << tempChunk.chunk.logicDataSize << endl;
            // cerr << setw(6) << "chunk hash = " << endl;
            // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            // cerr << setw(6) << "chunk key = " << endl;
            // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);

            // string tempHashForCache;
            // tempHashForCache.resize(CHUNK_HASH_SIZE);
            // memcpy(&tempHashForCache[0], tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            // if (kCache.existsKeyinCache(tempHashForCache)) {
            //     string hitCacheTemp = kCache.getKeyFromCache(tempHashForCache);
            //     memcpy(tempChunk.chunk.encryptKey, &hitCacheTemp[0], CHUNK_ENCRYPT_KEY_SIZE);
            //     insertMQToPOW(tempChunk);
            // } else {
            batchList.push_back(tempChunk);
            memcpy(chunkHash + batchNumber * CHUNK_HASH_SIZE, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            batchNumber++;
            // }
        }
        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            int batchedKeySize = 0;

            if (!keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize)) {
                cerr << "KeyClient : error get key for " << setbase(10) << batchNumber << " chunks" << endl;
                batchList.clear();
                memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
                memset(chunkKey, 0, CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_);
                batchNumber = 0;
            } else {
                cerr << "KeyClient : key exchange for " << setbase(10) << batchNumber << " chunks over" << endl;
                // cerr << "KeyClient : batchlist size = " << batchList.size() << " , batch number = " << batchNumber << endl;
                // cerr << setw(6) << "key exchange chunk hash = " << endl;
                // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, chunkHash, CHUNK_HASH_SIZE * batchNumber);
                // cerr << setw(6) << "key exchange chunk key = " << endl;
                // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, chunkKey, CHUNK_ENCRYPT_KEY_SIZE * batchNumber);

                for (int i = 0; i < batchNumber; i++) {
                    // cerr << "KeyClient : current chunk ID = " << batchList[i].chunk.ID << " chunk data size = " << batchList[i].chunk.logicDataSize << endl;
                    // cerr << setw(6) << "chunk hash = " << endl;
                    // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, batchList[i].chunk.chunkHash, CHUNK_HASH_SIZE);
                    // cerr << setw(6) << "chunk key = " << endl;
                    // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, batchList[i].chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                    memcpy(batchList[i].chunk.encryptKey, chunkKey + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
                    // string tempHash((char*)batchList[i].chunk.chunkHash, CHUNK_HASH_SIZE);
                    // string tempKey((char*)batchList[i].chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                    // kCache.insertKeyToCache(tempHash, tempKey);

                    // cerr << "KeyClient : new current chunk ID = " << batchList[i].chunk.ID << " chunk data size = " << batchList[i].chunk.logicDataSize << endl;
                    // cerr << setw(6) << "chunk hash = " << endl;
                    // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, batchList[i].chunk.chunkHash, CHUNK_HASH_SIZE);
                    // cerr << setw(6) << "chunk key = " << endl;
                    // PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, batchList[i].chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                    Data_t newDataToInsert;
                    memcpy(&newDataToInsert, &batchList[i], sizeof(Data_t));
                    if (encodeChunk(newDataToInsert)) {
                        insertMQToPOW(newDataToInsert);
                    } else {
                        cerr << "KeyClient : encode chunk error, exiting" << endl;
                        exit(0);
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

bool keyClient::encodeChunk(Data_t& newChunk)
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
