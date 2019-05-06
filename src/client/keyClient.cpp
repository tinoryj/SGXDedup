//
// Created by a on 11/17/18.
//

#include "keyClient.hpp"
#include "openssl/rsa.h"

#include <bits/stdc++.h>
#include "chrono"
#include "unistd.h"

#include <sys/time.h>

extern Configure config;
extern util::keyCache kCache;

keyClient::keyClient(){
    _inputMQ.createQueue(CHUNKER_TO_KEYCLIENT_MQ,READ_MESSAGE);
    _outputMQ.createQueue(KEYCLIENT_TO_ENCODER_MQ,WRITE_MESSAGE);
    _keyBatchSizeMin=(int)config.getKeyBatchSizeMin();
    _keyBatchSizeMax=(int)config.getKeyBatchSizeMax();
    _socket.init(CLIENTTCP,config.getKeyServerIP(0),config.getKeyServerPort(0));
    kmServer server(_socket);
    powSession *session=server.authkm();
    if(session!= nullptr){
        _trustdKM= true;
        _crypto.setSymKey((const char*)session->smk,16,(const char*)session->smk,16);
        delete session;
    }else{
        _trustdKM=false;
    }
}

keyClient::~keyClient(){
}

void keyClient::run() {

    timeval st,ed;

    while (1) {
        vector<Chunk> chunkList;
        string segmentKey;
        string minHash;
        minHash.resize(32);
        Chunk tmpchunk;
        char mask[32];
        int segmentSize;
        int minHashIndex, it;

        memset(mask, '0', sizeof(mask));
        memset(&minHash[0], 255, sizeof(char) * minHash.length());
        chunkList.clear();

        for (it = segmentSize = minHashIndex = 0; segmentSize < _keyBatchSizeMax; it++) {
            if (!_inputMQ.pop(tmpchunk)) {
                break;
            };
            if(it==0)
                gettimeofday(&st,NULL);
            chunkList.push_back(tmpchunk);

            segmentSize += chunkList[it].getLogicDataSize();
            if (memcmp((void *) chunkList[it].getChunkHash().c_str(), &minHash[0], sizeof(minHash)) < 0) {
                memcpy(&minHash[0], chunkList[it].getChunkHash().c_str(), sizeof(minHash));
                minHashIndex = it;
            }

            if (memcmp(chunkList[it].getChunkHash().c_str() + (32 - 9), mask, 9) == 0 &&
                segmentSize > _keyBatchSizeMax) {
                break;
            }
        }

        if (chunkList.empty()) {
            continue;
        }

        if (kCache.existsKeyinCache(minHash)) {
            segmentKey = kCache.getKeyFromCache(minHash);
            for (auto it:chunkList) {
                it.editEncryptKey(segmentKey);
            }
            continue;
        }
        segmentKey = keyExchange(chunkList[minHashIndex]);

        //std::cout<<"get key : "<<segmentKey<<endl;

        //write to hash cache
        kCache.insertKeyToCache(minHash, segmentKey);

        for (auto it:chunkList) {
            it.editEncryptKey(segmentKey);
            this->insertMQ(it);
        }
        gettimeofday(&ed,NULL);
        //std::cerr<<"keyClient time scale : "<<ed.tv_usec-st.tv_usec<<endl;
    }
    //close ssl connection
}

string keyClient::keyExchange(Chunk champion){
    string key,buffer;
    key.resize(16);
    if (!_trustdKM){
        printf("keyClient: can not do keyExchange before peer trusted\n");
        return key;
    }
    if(!_socket.Send(champion.getChunkHash())){
        printf("keyClient: socket error\n");
        return key;
    }
    if(!_socket.Recv(buffer)){
        printf("keyClient: socket error\n");
        return key;
    }
    _crypto.cbc128_decrypt(buffer,key);
    return key;
}

bool keyClient::insertMQ(Chunk &newChunk) {
    _outputMQ.push(newChunk);
    Recipe_t* _recipe=newChunk.getRecipePointer();
    keyRecipe_t *kr=&_recipe->_k;
    kr->_body[newChunk.getID()]._chunkKey=newChunk.getEncryptKey();
}