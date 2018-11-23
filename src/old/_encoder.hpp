#ifndef _ENCODER_HPP
#define _ENCODER_HPP

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
//#include "leveldb/db.h"

class _Encoder {
    private:
        _messageQueue _inputMQ;
        _messageQueue _outputMQ;
        std::ofstream encodeRecoder;
        // any additional info
    public:
        _Encoder();
        ~_Encoder();
        bool extractMQ(Chunk &data);
        bool insertMQ(Chunk data); 
        virtual bool getKey(Chunk newChunk) = 0;
        virtual bool encodeChunk(Chunk newChunk) = 0;
        virtual bool outputEncodeRecoder() = 0;
        
    
        _messageQueue getInputMQ();
        _messageQueue getOutputMQ();
        // any additional functions
};

#endif