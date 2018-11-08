#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <bitset>
#include <cstdlib>
#include <cmath>
#include <set>
#include <list>
#include <deque>
#include <map>
#include <queue>
#include <fstream>

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "leveldb/db.h"

class _Encoder {
    private:
        MessageQueue<Chunk> _inputMQ;
        MessageQueue<Chunk> _outputMQ;
        std::ofstream encodeRecoder;
        // any additional info
    public:
        _Encoder();
        ~_Encoder();
        bool extractMQ();
        bool insertMQ(); 
        virtual bool getKey(Chunk newChunk) = 0;
        virtual bool encodeChunk(Chunk newChunk) = 0;
        virtual bool outputEncodeRecoder() = 0;
        
        void getInputMQ();
        void getOutputMQ();
        /*
        MessageQueue getInputMQ();
        MessageQueue getOutputMQ();
        */
        // any additional functions
};

void _Encoder::getInputMQ(){
    _inputMQ.createQueue("Chunker2Encoder",READ_MESSAGE,100);
}

void _Encoder::getOutputMQ(){
    _inputMQ.createQueue("Encoder2SGX",WRITE_MESSAGE,100);
}