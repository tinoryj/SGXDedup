//
// Created by a on 11/17/18.
//

#include "keyClient.hpp"
#include "openssl/rsa.h"

extern Configure config;
extern keyCache kCache;

keyClient::keyClient(encoder* encoderObjTemp)
{
    encoderObj = encoderObjTemp;
    cryptoObj = new CryptoPrimitive();
    keyBatchSizeMin = (int)config.getKeyBatchSizeMin();
    keyBatchSizeMax = (int)config.getKeyBatchSizeMax();
    socket.init(CLIENT_TCP, config.getKeyServerIP(), config.getKeyServerPort());
    kmServer server(socket);
    powSession* session = server.authkm();
    if (session != nullptr) {
        trustdKM = true;
        cryptoObj.setSymKey((const char*)session->smk, 16, (const char*)session->smk, 16);
        delete session;
    } else {
        trustdKM = false;
        delete session;
    }
}

keyClient::~keyClient()
{

    if (cryptoObj != NULL) {
        delete cryptoObj;
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

        for (int it = segmentSize = minHashIndex = 0; segmentSize < keyBatchSizeMax; it++) {
            if (inputMQ.done_ && !extractMQFromChunker(tempchunk)) {
                exitFlag = true;
                break;
            };
            chunkList.push_back(tempchunk);

            segmentSize += chunkList[it].logicDataSize;
            if (memcmp((void*)chunkList[it].chunkHash, &minHash[0], sizeof(minHash)) < 0) {
                memcpy(&minHash[0], chunkList[it].chunkHash, sizeof(minHash));
                minHashIndex = it;
            }

            if (memcmp(chunkList[it].chunkHash + (32 - 9), mask, 9) == 0 && segmentSize > keyBatchSizeMax) {
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
        }
        if (exitFlag) {
            encoderObj->editJobDoneFlag();
            pthread_exit(NULL);
        }
    }
    //close ssl connection
}

string keyClient::keyExchange(Chunk_t champion)
{
    string key, buffer;
    key.resize(16);
    while (!trustdKM)
        ;
    string minHash(champion.chunkHash, CHUNK_HASH_SIZE);
    if (!socket.Send(minHash)) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    if (!socket.Recv(buffer)) {
        cerr "keyClient: socket error" << endl;
        exit(0);
    }
    cryptoObj.cbc128_decrypt(buffer, key);
    u_char returnValue_char[CHUNK_ENCRYPT_KEY_SIZE];
    memcpy(returnValue_char, key.c_str(), 16);
    memcpy(returnValue_char + 16, key.c_str(), 16);
    string returnValue(returnValue_char, CHUNK_ENCRYPT_KEY_SIZE);
    return returnValue;
}

bool keyClient::insertMQFromChunker(Chunk_t newChunk)
{
    return inputMQ.push(newChunk);
}

bool keyClient::extractMQFromChunker(Chunk_t newChunk)
{
    return inputMQ.pop(newChunk);
}

bool keyClient::insertMQtoEncoder(Chunk_t newChunk)
{
    return encoderObj->insertMQFromKeyClient(newChunk);
}

bool keyClient::editJobDoneFlag()
{
    inputMQ.done_ = true;
}