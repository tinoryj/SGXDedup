#include "chunk.hpp"

uint64_t Chunk::getID() {
    
    return this->_ID;
}

uint64_t Chunk::getType() {
    
    return this->_type;
}

uint64_t Chunk::getLogicDataSize() {
    
    return this->_logicDataSize;
}

std::string Chunk::getLogicData() {
    
    return this->_logicData;
}

std::string Chunk::getChunkHash() {
    
    return this->_chunkHash;
}

std::string Chunk::getMetaData() {
    
    return this->_metaData;
}

std::string Chunk::getEncryptKey() {
    
    return this->_encryptKey;
}

bool Chunk::editType(uint64_t type) {
    
    this->_type = type;
}

/*
bool editLogicData(std::string newLogicData){
    this->_logicData=newLogicData;
}
bool editLogicDataSize(uint64_t newSize){
    this->_logicDataSize=newSize;
}
*/
bool Chunk::editLogicData(std::string newLogicDataContent, uint64_t newLogicDataSize) {
    
    this->_logicDataSize = newLogicDataSize;
    newLogicDataContent.resize(newLogicDataSize);
    this->_logicData = newLogicDataContent;
}

bool Chunk::editEncryptKey(std::string newKey) {
    
    this->_encryptKey = newKey;
}