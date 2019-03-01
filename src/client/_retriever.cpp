#include "_retriever.hpp"



_Retriever::_Retriever(string fileName) {
    _retrieveFile.open(fileName,ios::out|ios::binary);
    _inputMQ.createQueue(DECODER_TO_RETRIEVER,READ_MESSAGE);
}

_Retriever::~_Retriever() {
    _retrieveFile.close();
}

_messageQueue _Retriever::getInputMQ() {
    return _inputMQ;
}

bool _Retriever::extractMQ(Chunk &chunk) {
    return _inputMQ.pop(chunk);
}

bool _Retriever::extractMQ(int &chunkCnt) {
    return _inputMQ.pop(chunkCnt);
}