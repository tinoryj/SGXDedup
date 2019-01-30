//
// Created by a on 1/30/19.
//

#include "_decoder.hpp"

_Decoder::_Decoder() {
    _inputMQ.createQueue(RECEIVER_TO_DECODER_MQ,READ_MESSAGE);
    _outputMQ.createQueue(DECODER_TO_RETRIEVER,WRITE_MESSAGE);


    //decoderRecoder?
}

_Decoder::~_Decoder() {}


bool _Decoder::extractMQ(Chunk &recvChunk) {
    _inputMQ.pop(recvChunk);
}

bool _Decoder::extractMQ(string &keyRecipe) {
    _inputMQ.pop(keyRecipe);
}

bool _Decoder::insertMQ(Chunk &chunk) {
    _outputMQ.push(chunk);
}

_messageQueue _Decoder::getInputMQ() {
    return _inputMQ;
}

_messageQueue _Decoder::getOutputMQ() {
    return _outputMQ;
}

