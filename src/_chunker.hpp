#ifndef _CHUNKER_HPP
#define _CHUNKER_HPP

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

//#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"

class _Chunker {
private:
    std::ifstream _chunkingFile;
    // any additional info 
public:
    _Chunker();

    _Chunker(std::string path);

    ~_Chunker();

    void loadChunkFile(std::string path);

    virtual bool chunking() = 0;

    std::ifstream &getChunkingFile();

    bool insertMQ();
    // any additional functions
};

std::ifstream &_Chunker::getChunkingFile() {

    return _chunkingFile;
}

void _Chunker::loadChunkFile(std::string path) {
    if (_chunkingFile.is_open()) {
        _chunkingFile.close();
    }
    _chunkingFile.open(path, std::ios::binary);
    if (!_chunkingFile.is_open()) {
        std::cerr << "No such file: " << path << "\n";
        exit(1);
    }
}

_Chunker::~_Chunker() {
    if (_chunkingFile.is_open()) {
        _chunkingFile.close();
    }
}

_Chunker::_Chunker() {}

_Chunker::_Chunker(std::string path) {
    loadChunkFile(path);
}

bool _Chunker::insertMQ() {

}


#endif
