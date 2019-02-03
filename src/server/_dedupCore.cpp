#include "_dedupCore.hpp"

using namespace std;


_DedupCore::_DedupCore() {
    _inputMQ.createQueue(DATASR_TO_DEDUPCORE_MQ, READ_MESSAGE);
    _outputMQ.createQueue(DEDUPCORE_TO_STORAGECORE_MQ, WRITE_MESSAGE);
}

_DedupCore::~_DedupCore() {}



bool _DedupCore::insertMQ(epoll_message &msg) {
    _inputMQ.push(msg);
    return true;
}

bool _DedupCore::extractMQ(epoll_message &msg) {
    return _outputMQ.pop(msg);
}

_messageQueue _DedupCore::getOutputMQ() {
    return _outputMQ;
}

_messageQueue _DedupCore::getInputMQ() {
    return _inputMQ;
}