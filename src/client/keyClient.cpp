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
        cryptoObj_->setSymKey((const char*)session->smk, 16, (const char*)session->smk, 16);
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
        vector<Chunk_t> chunkList;
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
                memcpy(it.encryptKey, segmentKey.c_str(), CHUNK_ENCRYPT_KEY_SIZE);
                //memcpy(chunkerObj_->getRecipeList[it.ID].chunkKey, it.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
            }
            continue;
        }
        segmentKey = keyExchange(chunkList[minHashIndex]);

        //std::cout<<"get key : "<<segmentKey<<endl;

        //write to hash cache
        kCache.insertKeyToCache(minHash, segmentKey);
        for (auto it : chunkList) {
            memcpy(it.encryptKey, segmentKey.c_str(), CHUNK_ENCRYPT_KEY_SIZE);
            insertMQtoEncoder(it);
            //memcpy(chunkerObj_->getRecipeList[it.ID].chunkKey, it.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
        }
        if (exitFlag) {
            encoderObj_->editJobDoneFlag();
            pthread_exit(NULL);
        }
    }
    //close ssl connection
}

string keyClient::keyExchange(Chunk_t champion)
{
    string key, buffer;
    key.resize(16);
    while (!trustdKM_)
        ;
    string minHash;
    minHash.resize(CHUNK_HASH_SIZE);
    memcpy(&minHash[0], champion.chunkHash, CHUNK_HASH_SIZE);
    if (!socket_.Send(minHash)) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    if (!socket_.Recv(buffer)) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    cryptoObj_->cbc128_decrypt(buffer, key);
    u_char returnValue_char[CHUNK_ENCRYPT_KEY_SIZE];
    memcpy(returnValue_char, key.c_str(), 16);
    memcpy(returnValue_char + 16, key.c_str(), 16);
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