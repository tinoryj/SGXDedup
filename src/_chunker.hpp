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

class _Chunker {
    private:
        std::ifstream _chunkingFile;
        // any additional info 
    public:
        _Chunker();
        ~_Chunker();
        virtual bool chunking() = 0;
        std::ifstream& getChunkingFile();
        bool insertMQ();
        // any additional functions
};

std::ifstream& _Chunker::getChunkingFile() {

    return _chunkingFile;
}

bool _Chunker::insertMQ() {

}
