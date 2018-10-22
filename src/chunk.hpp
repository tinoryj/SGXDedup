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

class Chunk {
    private: 
        uint64_t _ID;
        uint64_t _type;
        uint64_t _logicDataSize;
        std::string _logicData;
        std::string _metaData;
        std::string _chunkHash;
        std::string _encryptKey;
        // any additional info of chunk
        
    public:
        Chunk(uint64_t ID, uint64_t type, uint64_t logicDataSize, std::string logicData, std::string metaData, std::string chunkHash);
        ~Chunk();
        uint64_t getID();
        uint64_t getType();
        uint64_t getLogicDataSize();
        std::string getLogicData();
        std::string getChunkHash();
        std::string getMetaData();
        std::string getEncryptKey();
        bool editType(uint64_t type);
        bool editLogicData(std::string newLogicData);
        bool editLogicDataSize(uint64_t newSize);
        bool editEncryptKey(std::string newKey);
        // any additional function of chunk

};

Chunk::Chunk(uint64_t ID, uint64_t type, uint64_t logicDataSize, std::string logicData, std::string metaData, std::string chunkHash) {
    _ID = ID;
    _type = type;
    _logicDataSize = logicDataSize;
    _logicData = logicData;
    _metaData = metaData;
    _chunkHash = chunkHash;
}

uint64_t Chunk::getID() {

    return _ID;
}

uint64_t Chunk::getType() {

    return _type;
}

uint64_t Chunk::getLogicDataSize() {

    return _logicDataSize;
}

std::string Chunk::getLogicData() {

    return _logicData;
}

std::string Chunk::getChunkHash() {

    return _chunkHash;
}

std::string Chunk::getMetaData() {

    return _metaData;
}

std::string Chunk::getEncryptKey() {

    return _encryptKey;
}

bool Chunk::editType(uint64_t type) {

    _type = type;
    if (_type == type) {
        return true;
    }
    else {
        return false;
    }
}

bool Chunk::editLogicData(std::string newLogicData) {

    _logicData = newLogicData;
    if (_logicData == newLogicData) {
        return true;
    }
    else {
        return false;
    }
}

bool Chunk::editLogicDataSize(uint64_t newSize) {

    _logicDataSize = newSize;
    if (_logicDataSize == newSize) {
        return true;
    }
    else {
        return false;
    }
}

bool Chunk::editEncryptKey(std::string newKey) {

    _encryptKey = newKey;
    if (_encryptKey == newKey) {
        return true;
    }
    else {
        return false;
    }
}