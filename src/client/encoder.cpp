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
    _messageQueue in=getInputMQ();
    _messageQueue out=getOutputMQ();

    while(1){
        Chunk tmpChunk;
        in.pop(tmpChunk);
        encodeChunk(tmpChunk);
        out.push(tmpChunk);
    }
}


bool encoder::encodeChunk(Chunk& tmpChunk){
    string newLogicData;
    _cryptoObj->encryptWithKey(tmpChunk.getLogicData(),tmpChunk.getEncryptKey(),newLogicData);
    tmpChunk.editLogicData(newLogicData,newLogicData.length());
}
