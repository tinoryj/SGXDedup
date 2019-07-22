//
// Created by a on 1/30/19.
//

#include "decoder.hpp"

extern Configure config;

decoder::decoder()
{
    cryptoObj_ = new CryptoPrimitive();
}

decoder::~decoder() {}

bool decoder::getKey(Chunk_t& newChunk)
{
    string key = keyRecipe_.at(newChunk.getChunkHash());
    newChunk.editEncryptKey(key);
}

bool decoder::decodeChunk(Chunk_t& newChunk)
{
    cryptoObj_->chunk_decrypt(newChunk);
}

void decoder::run()
{
    string buffer, buffer1;
    Recipe_t recipe;
    //keyRecipe_t recipe;
    while (!this->extractMQ(buffer))
        ;
    deserialize(buffer, recipe);

    tmpMQ.push(recipe._f._body.size());

    /**********************/
    //temp implement
    char recipekey[128];
    memset(recipekey, 0, sizeof(recipekey));
    cryptoObj_->setSymKey(recipekey, 128, recipekey, 128);
    /*********************/

    cryptoObj_->recipe_decrypt(recipe._kencrypted, recipe._k);
    for (auto it : recipe._k._body) {
        this->_keyRecipe.insert(make_pair(it._chunkHash, it._chunkKey));
    }
    int maxThread = config.getDecoderThreadLimit();
    for (int i = 0; i < maxThread; i++) {
        boost::thread th(boost::bind(&decoder::runDecode, this));
        th.detach();
    }
}

void decoder::runDecode()
{
    Chunk_t tmpChunk;
    while (true) {
        while (!this->extractMQ(tmpChunk))
            ;
        tmpChunk.editEncryptKey(_keyRecipe.at(tmpChunk.getChunkHash()));
        this->decodeChunk(tmpChunk);
        this->insertMQ(tmpChunk);
    }
}

bool decoder::outputDecodeRecoder()
{
    return true;
}