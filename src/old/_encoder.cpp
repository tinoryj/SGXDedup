#include "_encoder.hpp"

extern Configure config;

_messageQueue _Encoder::getInputMQ(){
	return _inputMQ;
}

_messageQueue _Encoder::getOutputMQ(){
	return _outputMQ;
}

bool _Encoder::extractMQ(Chunk &data){
    _inputMQ.pop(data);
    return true;
}

bool _Encoder::insertMQ(Chunk data){
    _inputMQ.push(data);
    return true;
}

_Encoder::_Encoder(){
    _inputMQ.createQueue("keyClient to encoder",READ_MESSAGE,config.getMessageQueueCnt(),config.getMessageQueueUnitSize());
    _outputMQ.createQueue("encoder to sender",WRITE_MESSAGE,config.getMessageQueueCnt(),config.getMessageQueueUnitSize());
    //ofstream=
}

_Encoder::~_Encoder(){
    //close mq
}