#include "_receiver.hpp"


_Receiver::_Receiver() {
    _outputMQ.createQueue(RECEIVER_TO_DECODER_MQ,WRITE_MESSAGE);
}

bool _Receiver::insertMQ(Chunk &chunk) {
    _outputMQ.push(chunk);
}

bool _Receiver::insertMQ(string &keyRecipe) {
    _outputMQ.push(keyRecipe);
}

_Receiver::~_Receiver() {

}

_messageQueue _Receiver::getOutputMQ() {
    return _outputMQ;
}