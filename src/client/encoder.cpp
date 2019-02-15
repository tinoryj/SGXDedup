//
// Created by a on 11/17/18.
//

#include "encoder.hpp"


encoder::encoder(){
    _cryptoObj=new CryptoPrimitive();
}

encoder::~encoder(){
    if(_cryptoObj!=NULL){
        delete _cryptoObj;
    }
}

void encoder::run(){

    while(1){
        Chunk tmpChunk;
        if(!extractMQ(tmpChunk)){
            continue;
        }
        encodeChunk(tmpChunk);
        insertMQ(tmpChunk);
    }
}


bool encoder::encodeChunk(Chunk& tmpChunk){
    _cryptoObj->chunk_encrypt(tmpChunk);
}
