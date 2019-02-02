#include "_sender.hpp"

extern Configure config;

_Sender::_Sender() {
    getInputMQ();
}

_Sender::~_Sender() {}


bool _Sender::extractMQ(Chunk &tmpChunk) {
    return _inputMQ.pop(tmpChunk);
}

void _Sender::getInputMQ() {
    _inputMQ.createQueue(SENDER_IN_MQ, READ_MESSAGE);
}
