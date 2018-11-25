//
// Created by a on 11/17/18.
//


#include "configure.hpp"

Configure::~Configure(){}
Configure::Configure(){}
Configure::Configure(std::string path){
    this->readConf(path);
}

void Configure::readConf(std::string path) {
    using namespace boost;
    using namespace boost::property_tree;
    ptree root;
    read_json<ptree>(path, root);


    //Chunker Configure
    _runningType = root.get<uint64_t>\
    ("ChunkerConfig._runningType");
    _chunkingType = root.get<uint64_t>\
    ("ChunkerConfig._chunkingType");
    _maxChunkSize = root.get<uint64_t>\
    ("ChunkerConfig._maxChunkSize");
    _minChunkSize = root.get<uint64_t>\
    ("ChunkerConfig._minChunkSize");
    _slidingWinSize = root.get<uint64_t>\
    ("ChunkerConfig._slidingWinSize");
    _segmentSize = root.get<uint64_t>\
    ("ChunkerConfig._segmentSize");
    _averageChunkSize = root.get<uint64_t>\
    ("ChunkerConfig._avgChunkSize");
    _ReadSize = root.get<uint64_t>\
    ("ChunkerConfig._ReadSize");

    //message queue Configure
    _messageQueueCnt = root.get<uint64_t>\
    ("MessageQueue._messageQueueCnt");
    _messageQueueUnitSize = root.get<uint64_t>\
    ("MessageQueue._messageQueueUnitSize");

    //Server Provider Configure
    _keyServerNumber = root.get<uint64_t>\
    ("KeyServerConfig._keyServerNumber");
    _keyBatchSizeMin = root.get<uint64_t>\
    ("KeyServerConfig._keyBatchSizeMin");
    _keyBatchSizeMax = root.get<uint64_t>\
    ("KeyServerConfig._keyBatchSizeMax");
    _keyCacheSize =root.get<uint64_t>\
    ("KeyServerConfig._keyCacheSize");

    _storageServerNumber = root.get<uint64_t>\
    ("SPConfig._storageServerNumber");

    _maxContainerSize = root.get<uint64_t>\
    ("SPConfig._maxContainerSize");

    //muti thread settings;
    _encodeThreadLimit=root.get<int>\
    ("mutiThread._encodeThreadLimit");
    _keyClientThreadLimit=root.get<int>\
    ("mutiThread._keyClientThreadLimit");
    _keyServerThreadLimit=root.get<int>\
    ("mutiThread._keyServerThreadLimit");

    //Key Server Congigure
    _keyServerIP.clear();
    for (ptree::value_type &it:root.get_child("KeyServerConfig._keyServerIP")) {
        _keyServerIP.push_back(it.second.data());
    }

    _keyServerPort.clear();
    for (ptree::value_type &it:root.get_child("KeyServerConfig._keyServerPort")) {
        _keyServerPort.push_back(it.second.get_value<int>());
    }


     _storageServerIP.clear();
    for (ptree::value_type &it:root.get_child("SPConfig._storageServerIP")) {
        _storageServerIP.push_back(it.second.data());
    }

    _storageServerPort.clear();
    for (ptree::value_type &it:root.get_child("SPConfig._storageServerPort")) {
        _storageServerPort.push_back(it.second.get_value<int>());
    }

}

uint64_t Configure::getRunningType() {

    return _runningType;
}

// chunking settings
uint64_t Configure::getChunkingType() {

    return _chunkingType;
}

uint64_t Configure::getMaxChunkSize() {

    return _maxChunkSize;
}

uint64_t Configure::getMinChunkSize() {

    return _minChunkSize;
}

uint64_t Configure::getAverageChunkSize() {

    return _averageChunkSize;
}

uint64_t Configure::getSlidingWinSize() {

    return _slidingWinSize;
}

uint64_t Configure::getSegmentSize() {

    return _segmentSize;
}

uint64_t Configure::getReadSize() {
    return _ReadSize;
}

// message queue settions
uint64_t Configure::getMessageQueueCnt() {

    return _messageQueueCnt;
}

uint64_t Configure::getMessageQueueUnitSize() {

    return _messageQueueUnitSize;
}

// key management settings
uint64_t Configure::getKeyServerNumber() {

    return _keyServerNumber;
}


uint64_t Configure::getKeyBatchSizeMin(){

    return _keyBatchSizeMin;
}

uint64_t Configure::getKeyBatchSizeMax(){

    return _keyBatchSizeMax;
}

uint64_t Configure::getKeyCacheSize() {
    return  _keyCacheSize;
}

/*
std::vector<std::string> Configure::getkeyServerIP() {

    return _keyServerIP;
}

std::vector<int> Configure::getKeyServerPort() {

    return _keyServerPort;
}

*/

std::string Configure::getKeyServerIP(){
    return _keyServerIP[0];
}

int Configure::getKeyServerPort(){
    return _keyServerPort[0];
}

//muti thread settings
int Configure::getEncoderThreadLimit() {
    return _encodeThreadLimit;
}

int Configure::getKeyClientThreadLimit() {
    return _keyClientThreadLimit;
}

int Configure::getKeyServerThreadLimit() {
    return _keyServerThreadLimit;
}

// storage management settings
uint64_t Configure::getStorageServerNumber() {

    return _storageServerNumber;
}

std::string Configure::getStorageServerIP() {

    return _storageServerIP[0];
}
/*
std::vector<std::string> Configure::getStorageServerIP() {

    return _storageServerIP;
}*/

int Configure::getStorageServerPort() {

    return _storageServerPort[0];
}

/*
std::vector<int> Configure::getStorageServerPort() {

    return _storageServerPort;
}*/

uint64_t Configure::getMaxContainerSize() {

    return _maxContainerSize;
}
