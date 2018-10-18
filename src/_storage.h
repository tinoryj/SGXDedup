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

#include "_messageQueue.h"
#include "_configure.h"
#include "_chunk.h"
#include "leveldb/db.h"
using namespace std;

class StorageCore {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        vector<leveldb::DB> _dbSet;
        vector<ifstream> _intputContainerSet;
        vector<ofstream> _outputContainerSet; 
        // any additional info
    public:
        StorageCore();
        ~StorageCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool createContainer() = 0;
        virtual bool writeContainer() = 0;
        virtual bool readContainer() = 0;
        // any additional functions
};