//
// Created by a on 11/17/18.
//

#include "keyClient.hpp"
#include "openssl/rsa.h"

extern Configure config;
extern keyCache kCache;

keyClient::keyClient(encoder* encoderObjTemp)
{
    encoderObj_ = encoderObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSizeMin_ = (int)config.getKeyBatchSizeMin();
    keyBatchSizeMax_ = (int)config.getKeyBatchSizeMax();
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
    }
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}

void keyClient::run()
{
    while (true) {
        ChunkList_t chunkList;
        string segmentKey;
        string minHash;
        minHash.resize(32);
        Chunk_t tempchunk;
        char mask[32];
        int segmentSize;
        int minHashIndex;
        bool exitFlag = false;
        memset(mask, '0', sizeof(mask));
        memset(&minHash[0], 255, sizeof(char) * minHash.length());
        chunkList.clear();

        for (int it = segmentSize = minHashIndex = 0; segmentSize < keyBatchSizeMax_; it++) {
            if (inputMQ_.done_ && !extractMQFromChunker(tempchunk)) {
                exitFlag = true;
                break;
            };
            chunkList.push_back(tempchunk);

            segmentSize += chunkList[it].logicDataSize;
            if (memcmp((void*)chunkList[it].chunkHash, &minHash[0], sizeof(minHash)) < 0) {
                memcpy(&minHash[0], chunkList[it].chunkHash, sizeof(minHash));
                minHashIndex = it;
            }

            if (memcmp(chunkList[it].chunkHash + (32 - 9), mask, 9) == 0 && segmentSize > keyBatchSizeMax_) {
                break;
            }
        }

        if (chunkList.empty()) {
            continue;
        }

        if (kCache.existsKeyinCache(minHash)) {
            segmentKey = kCache.getKeyFromCache(minHash);
            for (auto it : chunkList) {
                memcpy(it.encryptKey, &segmentKey[0], CHUNK_ENCRYPT_KEY_SIZE);
            }
            continue;
        }
        segmentKey = keyExchange(chunkList[minHashIndex]);

        //write to hash cache
        kCache.insertKeyToCache(minHash, segmentKey);
        for (auto it : chunkList) {
            memcpy(it.encryptKey, &segmentKey[0], CHUNK_ENCRYPT_KEY_SIZE);
            insertMQtoEncoder(it);
        }
        if (exitFlag) {
            encoderObj_->editJobDoneFlag();
            break;
        }
    }
    pthread_exit(NULL);
    //close ssl connection
}

string keyClient::keyExchange(Chunk_t champion)
{
    while (!trustdKM_)
        ;
    u_char minHash[CHUNK_HASH_SIZE];
    memcpy(minHash, champion.chunkHash, CHUNK_HASH_SIZE);
    u_char sendHash[CHUNK_HASH_SIZE];
    cryptoObj_->keyExchangeEncrypt(minHash, CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    if (!socket_.Send(sendHash, CHUNK_HASH_SIZE)) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    u_char* recvBuffer;
    int recvSize;
    if (!socket_.Recv(recvBuffer, recvSize) || recvSize != CHUNK_ENCRYPT_KEY_SIZE) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    u_char key[CHUNK_ENCRYPT_KEY_SIZE];
    cryptoObj_->keyExchangeDecrypt(recvBuffer, recvSize, keyExchangeKey_, keyExchangeKey_, key);
    u_char returnValue_char[CHUNK_ENCRYPT_KEY_SIZE];
    memcpy(returnValue_char, key, CHUNK_ENCRYPT_KEY_SIZE);
    string returnValue;
    returnValue.resize(CHUNK_ENCRYPT_KEY_SIZE);
    memcpy(&returnValue[0], returnValue_char, CHUNK_ENCRYPT_KEY_SIZE);
    return returnValue;
}

bool keyClient::insertMQFromChunker(Chunk_t newChunk)
{
    return inputMQ_.push(newChunk);
}

bool keyClient::extractMQFromChunker(Chunk_t newChunk)
{
    return inputMQ_.pop(newChunk);
}

bool keyClient::insertMQtoEncoder(Chunk_t newChunk)
{
    return encoderObj_->insertMQFromKeyClient(newChunk);
}

bool keyClient::editJobDoneFlag()
{
    inputMQ_.done_ = true;
    if (inputMQ_.done_) {
        return true;
    } else {
        return false;
    }
}