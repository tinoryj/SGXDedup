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

using namespace std;

class Chunker {
    friend class Configure;
    friend class MessageQueue;
    friend class Chunk;
    private:
        ifstream _chunkingFile;
        // any additional info 
    public:
        Chunker();
        ~Chunker();
        virtual bool chunking() = 0;
        bool insertMQ();
        // any additional functions
};