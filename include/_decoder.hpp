//
// Created by a on 1/30/19.
//

#ifndef GENERALDEDUPSYSTEM__DECODER_HPP
#define GENERALDEDUPSYSTEM__DECODER_HPP

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"

class _Decoder {
private:
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    std::ofstream decodeRecoder;
    // any additional info
public:
    _Decoder();
    ~_Decoder();

    //mod and new
    bool extractMQ(string &keyRecipe);
    bool extractMQ(Chunk &recvChunk);

    bool insertMQ(Chunk &chunk);
    virtual bool getKey(Chunk &newChunk) = 0;
    virtual bool decodeChunk(Chunk &newChunk) = 0;
    virtual bool outputDecodeRecoder() = 0;
    _messageQueue getInputMQ();
    _messageQueue getOutputMQ();
    // any additional functions
};


#endif //GENERALDEDUPSYSTEM_DECODER_HPP
