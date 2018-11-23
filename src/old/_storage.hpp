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

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "leveldb/db.h"

class _StorageCore {
    private:
        MessageQueue<Chunk> _inputMQ;
        MessageQueue<Chunk> _outputMQ;
        std::vector<leveldb::DB> _dbSet;
        std::vector<std::ifstream> _intputContainerSet;
        std::vector<std::ofstream> _outputContainerSet; 
        // any additional info
    public:
        _StorageCore();
        ~_StorageCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool createContainer() = 0;
        virtual bool writeContainer() = 0;
        virtual bool readContainer() = 0;
        MessageQueue<Chunk> getInputMQ();
        MessageQueue<Chunk> getOutputMQ();
        std::vector<leveldb::DB> _dbSet;
        std::vector<std::ifstream> getIntputContainerSet();
        std::vector<std::ofstream> getOutputContainerSet(); 
        // any additional functions
};