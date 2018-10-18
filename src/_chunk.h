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

using namespace std;

class Chunk {
    private: 
        uint64_t _ID;
        uint64_t _type;
        uint64_t _logicDataSize;
        string _logicData;
        string _metaData;
        string _chunkHash;
        string _encryptKey;
        // any additional info of chunk
        
    public:
        Chunk(uint64_t ID, uint64_t type, uint64_t logicDataSize, string logicData, string metaData, string chunkHash);
        ~Chunk();
        uint64_t getID();
        uint64_t getType();
        uint64_t getLogicDataSize();
        string getLogicData();
        string getChunkHash();
        bool editType(uint64_t type);
        bool editLogicData(string newLogicData);
        bool editLogicDataSize(uint64_t newSize);
        bool editEncryptKey(string newKey);
        // any additional function of chunk

};
