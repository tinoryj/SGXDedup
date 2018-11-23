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
	_messageQueue in=getInputMQs();
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
    tmpChunk.editLogicData(_cryptoObj->encryptWithKey(tmpChunk.getLogicData()),tmpChunk.getEncryptKey());
}
