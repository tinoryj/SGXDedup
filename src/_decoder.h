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

#include "_messageQueue.h"
#include "_configure.h"
#include "_chunk.h"
#include "leveldb/db.h"

using namespace std;

class Decoder {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        ofstream decodeRecoder;
        // any additional info
    public:
        Decoder();
        ~Decoder();
        bool extractMQ();
        bool insertMQ(); 
        virtual bool getKey(Chunk newChunk) = 0;
        virtual bool decodeChunk(Chunk newChunk) = 0;
        virtual bool outputDecodeRecoder() = 0;
        // any additional functions
};