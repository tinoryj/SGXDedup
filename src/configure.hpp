#ifndef CONFIGURE_HPP
#define CONFIGURE_HPP

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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define SIMPLE_CHUNKER 0
#define RABIN_CHUNKER 1
#define FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker

class Configure {
private:
    // following settings configure by macro set 
    uint64_t _runningType;      // localDedup \ serverDedup 
    uint64_t _maxThreadLimits;  // threadPool auto configure baseline
    // chunking settings
    uint64_t _chunkingType;     // varSize \ fixedSize \ simple
    uint64_t _maxChunkSize;
    uint64_t _minChunkSize;
    uint64_t _averageChunkSize;
    uint64_t _slidingWinSize;
    uint64_t _segmentSize;  // if exist segment function
    uint64_t _ReadSize;
    // key management settings
    uint64_t _keyServerNumber;
    std::vector<std::string> _keyServerIP;
    std::vector<int> _keyServerPort;
    // storage management settings
    uint64_t _storageServerNumber;
    std::vector<std::string> _storageServerIP;
    std::vector<int> _storageServerPort;
    uint64_t _maxContainerSize;
    // any additional settings
    std::ifstream _configureFile;
    
public:
    //  Configure(std::ifstream& confFile); // according to setting json to init configure class
    Configure(std::string path);

    Configure();

    ~Configure();

    void readConf(std::string path);

    uint64_t getRunningType();

    uint64_t getMaxThreadLimits();

    // chunking settings
    uint64_t getChunkingType();

    uint64_t getMaxChunkSize();

    uint64_t getMinChunkSize();

    uint64_t getAverageChunkSize();

    uint64_t getSlidingWinSize();

    uint64_t getSegmentSize();

    uint64_t getReadSize();

    // key management settings
    uint64_t getKeyServerNumber();

    std::vector<std::string> getkeyServerIP();

    std::vector<int> getKeyServerPort();

    // storage management settings
    uint64_t getStorageServerNumber();

    std::vector<std::string> getStorageServerIP();

    std::vector<int> getStorageServerPort();

    uint64_t getMaxContainerSize();
    // any additional configure function
    // any macro settings should be set here (using "const" to replace "#define")

};

/*
Configure::Configure(std::ifstream& confFile) {

    // implement read json file function
}
*/

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


    //Server Provider Configure
    _keyServerNumber = root.get<uint64_t>\
    ("KeyServerConfig._keyServerNumber");
    _storageServerNumber = root.get<uint64_t>\
    ("SPConfig._storageServerNumber");
    _maxThreadLimits = root.get<uint64_t>\
    ("ClientConfig._maxThreadLimits");

    _maxContainerSize = root.get<uint64_t>\
    ("SPConfig._maxContainerSize");


    //Key Server Congigure
    std::vector<std::string> _keyServerIP;
    for (ptree::value_type &it:root.get_child("KeyServerConfig._keyServerIP")) {
        _keyServerIP.push_back(it.second.data());
    }

    std::vector<int> _keyServerPort;
    for (ptree::value_type &it:root.get_child("KeyServerConfig._keyServerPort")) {
        _keyServerPort.push_back(it.second.get_value<int>());
    }

    
    std::vector<std::string> _storageServerIP;
    for (ptree::value_type &it:root.get_child("SPConfig._storageServerIP")) {
        _storageServerIP.push_back(it.second.data());
    }

    std::vector<int> _storageServerPort;
    for (ptree::value_type &it:root.get_child("SPConfig._storageServerPort")) {
        _storageServerPort.push_back(it.second.get_value<int>());
    }

}

uint64_t Configure::getRunningType() {

    return _runningType;
}

uint64_t Configure::getMaxThreadLimits() {

    return _maxThreadLimits;
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

// key management settings
uint64_t Configure::getKeyServerNumber() {

    return _keyServerNumber;
}

std::vector<std::string> Configure::getkeyServerIP() {

    return _keyServerIP;
}

std::vector<int> Configure::getKeyServerPort() {

    return _keyServerPort;
}

// storage management settings
uint64_t Configure::getStorageServerNumber() {

    return _storageServerNumber;
}

std::vector<std::string> Configure::getStorageServerIP() {

    return _storageServerIP;
}

std::vector<int> Configure::getStorageServerPort() {

    return _storageServerPort;
}

uint64_t Configure::getMaxContainerSize() {

    return _maxContainerSize;
}

#endif