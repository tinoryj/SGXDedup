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

class Configure {
    private: 
        // following settings configure by macro set 
        uint64_t _runningType; // localDedup \ serverDedup 
        uint64_t _maxThreadLimits; // threadPool auto configure baseline
        // chunking settings
        uint64_t _chunkingType; // varSize \ fixedSize
        uint64_t _maxChunkSize;
        uint64_t _minChunkSize;
        uint64_t _averageChunkSize;
        uint64_t _segmentSize; // if exist segment function
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
        Configure(std::ifstream& confFile); // according to setting json to init configure class
        ~Configure();
        
        uint64_t getRunningType(); 
        uint64_t getMaxThreadLimits(); 
        // chunking settings
        uint64_t getChunkingType(); 
        uint64_t getMaxChunkSize();
        uint64_t getMinChunkSize();
        uint64_t getAverageChunkSize();
        uint64_t getSegmentSize();
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

Configure::Configure(std::ifstream& confFile) {

    // implement read json file function
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

uint64_t Configure::getSegmentSize() {

    return _segmentSize;
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
