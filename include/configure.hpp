//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_CONFIGURE_HPP
#define GENERALDEDUPSYSTEM_CONFIGURE_HPP

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
    // chunking settings
    uint64_t _chunkingType;     // varSize \ fixedSize \ simple
    uint64_t _maxChunkSize;
    uint64_t _minChunkSize;
    uint64_t _averageChunkSize;
    uint64_t _slidingWinSize;
    uint64_t _segmentSize;  // if exist segment function
    uint64_t _ReadSize;     //128M per time

    // message queue settings
    uint64_t _messageQueueCnt;
    uint64_t _messageQueueUnitSize;

    // key management settings
    uint64_t _keyServerNumber;
    std::vector<std::string> _keyServerIP;
    std::vector<int> _keyServerPort;
    uint64_t _keyBatchSizeMax;
    uint64_t _keyBatchSizeMin;
    uint64_t _keyCacheSize;

    //muti thread settings
    int _encodeThreadLimit;
    int _keyClientThreadLimit;
    int _keyServerThreadLimit;


    //POW settings
    int _quoteType; //0x00 linkable; 0x01 unlinkable
    int _iasVersion;
    std::string _POWServerIp;
    int _POWServerPort;
    std::string _enclaveName;

    // storage management settings
    uint64_t _storageServerNumber;
    std::vector<std::string> _storageServerIP;
    std::vector<int> _storageServerPort;
    uint64_t _maxContainerSize;

    std::string _fileRecipeRootPath;
    std::string _keyRecipeRootPath;
    std::string _containerRootPath;

    std::string _fp2ChunkDBName;
    std::string _fn2MetaDBame;

    // any additional settings

public:

    std::string getFileRecipeRootPath();
    std::string getKeyRecipeRootPath();
    std::string getContainerRootPath();
    std::string getFp2ChunkDBName();
    std::string getFn2MetaDBName();
    int getChunkBatchSize();
    int getClientID();

    int getTimeOutScale();

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

    // message queue settions
    uint64_t getMessageQueueCnt();

    uint64_t getMessageQueueUnitSize();

    // key management settings
    uint64_t getKeyServerNumber();

    uint64_t getKeyBatchSizeMin();

    uint64_t getKeyBatchSizeMax();

    uint64_t getKeyCacheSize();

    std::string getKeyServerIP();
    //std::vector<std::string> getkeyServerIP();

    int getKeyServerPort();
    //std::vector<int> getKeyServerPort();

    //muti thread settings
    int getEncoderThreadLimit();
    int getKeyClientThreadLimit();
    int getKeyServerThreadLimit();

    // storage management settings
    uint64_t getStorageServerNumber();

    std::string getStorageServerIP();
    //std::vector<std::string> getStorageServerIP();

    int getStorageServerPort();
    //std::vector<int> getStorageServerPort();

    uint64_t getMaxContainerSize();

    uint8_t getQuoteType();
    int getIasVersion();
    // any additional configure function
    // any macro settings should be set here (using "const" to replace "#define")

    std::string getPOWServerIp();

    int getPOWServerPort();

    std::string getEnclaveName();
};


#endif //GENERALDEDUPSYSTEM_CONFIGURE_HPP
