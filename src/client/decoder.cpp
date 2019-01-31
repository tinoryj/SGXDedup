//
// Created by a on 1/30/19.
//

#include "decoder.hpp"

extern Configure config;

decoder::decoder() {
    _crypto=new CryptoPrimitive(LOW_SEC_PAIR_TYPE);
}

decoder::~decoder() {}

bool decoder::getKey(Chunk &newChunk) {
    string key=_keyRecipe.at(newChunk.getChunkHash());
    newChunk.editEncryptKey(key);
}

bool decoder::decodeChunk(Chunk &newChunk) {
    string chunkLogicData;
    _crypto->decryptWithKey(newChunk.getLogicData(),
                            newChunk.getEncryptKey(),
                            chunkLogicData);
    newChunk.editLogicData(chunkLogicData,chunkLogicData.length());
}

void decoder::run() {
    string buffer,buffer1;
    keyRecipe recipe;
    this->extractMQ(buffer);
    _crypto->decryptRecipe(buffer,buffer1);
    deserialize(buffer1,recipe);
    for(auto it:recipe._body){
        this->_keyRecipe.insert(make_pair(it._chunkHash,it._chunkKey));
    }
    int i,maxThread=config.getMaxThreadLimits();
    for(i=0;i<maxThread;i++){
        boost::thread th(boost::bind(&decoder::runDecode,this));
    }
}

void decoder::runDecode() {
    Chunk tmpChunk;
    while(1){
        this->extractMQ(tmpChunk);
        tmpChunk.editEncryptKey(_keyRecipe.at(tmpChunk.getChunkHash()));
        this->decodeChunk(tmpChunk);
        this->insertMQ(tmpChunk);
    }
}