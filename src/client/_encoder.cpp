//
// Created by a on 11/17/18.
//

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

bool _Encoder::insertMQ(Chunk &data){
    _outputMQ.push(data);
    return true;
}

_Encoder::_Encoder(){
    _inputMQ.createQueue(KEYCLIENT_TO_ENCODER_MQ,READ_MESSAGE);
    _outputMQ.createQueue(ENCODER_TO_POW_MQ,WRITE_MESSAGE);
}

_Encoder::~_Encoder(){
    //close mq
}