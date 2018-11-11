#include "_encoder.cpp"

MessageQueue _Encoder::getInputMQ(){
	return _inputMQ;
}

MessageQueue _Encoder::getOutputMQ(){
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

_Encoder()::_Encoder(){
    _inputMQ=new _messageQueue("keyClient to encoder",READ_MESSAGE,config.getMessageQueueCnt(),config.getMessageQueueUnitSize());
    _outputMQ=new _messageQueue("encoder to sender",WRITE_MESSAGE,config.getMessageQueueCnt(),config.getMessageQueueUnitSize());
    //ofstream=
}

_Encoder::~Encoder(){
    //close mq
}