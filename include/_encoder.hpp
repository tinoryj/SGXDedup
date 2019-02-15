//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM__ENCODER_HPP
#define GENERALDEDUPSYSTEM__ENCODER_HPP

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"

#ifdef DEBUG
#include <boost/thread.hpp>
#include <iostream>
#endif

class _Encoder {
private:
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    std::ofstream encodeRecoder;
    // any additional info

#ifdef DEBUG
    boost::shared_mutex _mtx;
#endif

public:
    _Encoder();
    ~_Encoder();
    bool extractMQ(Chunk &data);
    bool insertMQ(Chunk &data);
    //virtual bool getKey(Chunk newChunk) = 0;
    virtual bool encodeChunk(Chunk& newChunk) = 0;
    //virtual bool outputEncodeRecoder() = 0;


    _messageQueue getInputMQ();
    _messageQueue getOutputMQ();
    // any additional functions
};

#endif //GENERALDEDUPSYSTEM_ENCODER_HPP
