//
// Created by a on 11/17/18.
//

#include "encoder.hpp"


encoder::encoder(){
    _cryptoObj=new CryptoPrimitive(1);
}

encoder::~encoder(){
    if(_cryptoObj!=NULL){
        delete _cryptoObj;
    }
}

void encoder::run(){

    while(1){
        Chunk tmpChunk;
        extractMQ(tmpChunk);
        encodeChunk(tmpChunk);
        insertMQ(tmpChunk);
    }
}


bool encoder::encodeChunk(Chunk& tmpChunk){
    string newLogicData;
    _cryptoObj->encryptWithKey(tmpChunk.getLogicData(),tmpChunk.getEncryptKey(),newLogicData);
    tmpChunk.editLogicData(newLogicData,newLogicData.length());
}
