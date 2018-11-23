#include "_chunker.hpp"

std::ifstream &_Chunker::getChunkingFile() {

    if(!_chunkingFile.is_open()) {
        std::cerr<<"chunking file open failed\n";
        exit(1);
    }
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

