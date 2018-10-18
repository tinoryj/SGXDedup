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

using namespace std;

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
        vector<string> _keyServerIP;
        vector<int> _keyServerPort;
        // storage management settings
        uint64_t _storageServerNumber;
        vector<string> _storageServerIP;
        vector<int> _storageServerPort;
        uint64_t _maxContainerSize;
        // any additional settings
    public:
        Configure(); // according to setting json to init configure class
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
        vector<string> getkeyServerIP();
        vector<int> getKeyServerPort();
        // storage management settings
        uint64_t getStorageServerNumber();
        vector<string> getStorageServerIP();
        vector<int> getStorageServerPort();
        uint64_t getMaxContainerSize();
        // any additional configure function
        // any macro settings should be set here (using "const" to replace "#define")

};