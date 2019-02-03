#include "_storage.hpp"


_StorageCore::_StorageCore() {
    _inputMQ.createQueue(DEDUPCORE_TO_STORAGECORE_MQ, READ_MESSAGE);
    _outputMQ.createQueue(DATASR_IN_MQ, WRITE_MESSAGE);
}

_StorageCore::~_StorageCore() {}

_messageQueue _StorageCore::getOutputMQ() {
    return _outputMQ;
}

_messageQueue _StorageCore::getInputMQ() {
    return _inputMQ;
}

bool _StorageCore::insertMQ(epoll_message &msg) {
    _outputMQ.push(msg);
    return true;
}

bool _StorageCore::extractMQ(chunkList &chunks) {
    return _inputMQ.pop(chunks);
}