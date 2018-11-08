#ifndef ENCODE_HPP
#define ENCODE_HPP

#include "_encoder.hpp"
#include "CryptoPrimitive.hpp"
#include "keyManger.hpp"

extern Configure config;
extern keyManger keyobj;

class Encoder:public _Encoder{
private:
	CryptoPrimitive *_cryptoObj
	int _segmentSize;
	
public:
	void threadHandle();
};

Encoder::Encoder(){
	getInputMQ();
	getOutputMQ();
	_cryptoObj=new CryptoPrimitive();
}

Encoder::~Encoder(){
	if(_cryptoObj!=NULL){
		delete _cryptoObj;
	}
//	closeInputMQ();
//	closeOutputMQ();
}

void Encoder::threadHandle(){
	while(1){
		vector<Chunk>chunkList;
		Chunk tmpChunk;
		string segmentKey;
		string minHash="";
		int minHashIndex=0;

		//segment
		int it=0;
		while(it<_segmentSize){
			//may jam here
			_inputMQ.pop(tmpChunk);
			if(tmpChunk.getChunkHash()<minHash){
				minHash=tmpChunk.getChunkHash();
				minHashIndex=it;
			}
		}

		
		kexObj->keExchange(Chunk,segmentKey);
		for(it<(int)chunkList.size();it++){
			chunkList[it].editEncryptKey(segmentKey);
			encodeChunk(chunkList[it]);
			_output.push(chunkList[it]);
		}
	}
}

bool Encoder::encodeChunk(Chunk& tmpChunk){
	string newLogicData;
	_cryptoObj->encryptWithKey(tmpChunk.getLogicData(),tmpChunk.getEncryptKey(),newLogicData);
	tmpChunk.editLogicData(newLogicData,newLogicData.length());
}

#endif