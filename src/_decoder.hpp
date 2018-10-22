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

class _Decoder {
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        std::ofstream decodeRecoder;
        // any additional info
    public:
        _Decoder();
        ~_Decoder();
        bool extractMQ();
        bool insertMQ(); 
        virtual bool getKey(Chunk newChunk) = 0;
        virtual bool decodeChunk(Chunk newChunk) = 0;
        virtual bool outputDecodeRecoder() = 0;
        MessageQueue getInputMQ();
        MessageQueue getOutputMQ();
        // any additional functions
};