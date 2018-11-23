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

class _DedupCore {
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        std::vector<leveldb::DB> _dbSet;
        // any additional info
    public:
        _DedupCore();
        ~_DedupCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool dataDedup() = 0;
        MessageQueue getInputMQ();
        MessageQueue getOutputMQ();
        leveldb::DB getDBObject(int dbPosition);
        // any additional functions
};