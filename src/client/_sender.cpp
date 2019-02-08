#include "_sender.hpp"

extern Configure config;

_Sender::_Sender() {
    _inputMQ.createQueue(SENDER_IN_MQ, READ_MESSAGE);
}

_Sender::~_Sender() {}


bool _Sender::extractMQ(Chunk &tmpChunk) {
    return _inputMQ.pop(tmpChunk);
}

_messageQueue _Sender::getInputMQ() {
    return _inputMQ;
}
