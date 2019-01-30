#ifndef GENERALDEDUPSYSTEM__RETRIEVER_HPP
#define GENERALDEDUPSYSTEM__RETRIEVER_HPP

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"

extern Configure config;

class _Retriever {
    private:
        _messageQueue _inputMQ;
        // any additional info 
    public:
        std::ofstream _retrieveFile;
        _Retriever(string fileName);
        ~_Retriever();
        virtual bool Retrieve() = 0;
        bool extractMQ(Chunk &chunk);
        bool extractMQ(int &chunkCnt);
        _messageQueue getInputMQ();
        // any additional functions
};


#endif //GENERALDEDUPSYSTEM_RETRIEVER_HPP