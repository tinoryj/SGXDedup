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
