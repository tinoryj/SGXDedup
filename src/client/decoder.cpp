//
// Created by a on 1/30/19.
//

#include "decoder.hpp"

extern Configure config;

decoder::decoder() {
    _crypto=new CryptoPrimitive();
}

decoder::~decoder() {}

bool decoder::getKey(Chunk &newChunk) {
    string key=_keyRecipe.at(newChunk.getChunkHash());
    newChunk.editEncryptKey(key);
}

bool decoder::decodeChunk(Chunk &newChunk) {
    _crypto->chunk_decrypt(newChunk);
}

void decoder::run() {
    string buffer, buffer1;
    keyRecipe_t recipe;
    this->extractMQ(buffer);

    /**********************/
    //temp implement
    char recipekey[128];
    memset(recipekey, 0, sizeof(recipekey));
    _crypto->setSymKey(recipekey, 128, recipekey, 128);
    /*********************/

    _crypto->recipe_decrypt(buffer, recipe);
    for (auto it:recipe._body) {
        this->_keyRecipe.insert(make_pair(it._chunkHash, it._chunkKey));
    }
    int i, maxThread = config.getDecoderThreadLimit();
    for (i = 0; i < maxThread; i++) {
        boost::thread th(boost::bind(&decoder::runDecode, this));
        th.detach();
    }
}

void decoder::runDecode() {
    Chunk tmpChunk;
    while (1) {
        this->extractMQ(tmpChunk);
        tmpChunk.editEncryptKey(_keyRecipe.at(tmpChunk.getChunkHash()));
        this->decodeChunk(tmpChunk);
        this->insertMQ(tmpChunk);
    }
}


bool decoder::outputDecodeRecoder() {
    return true;
}