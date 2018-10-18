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

class DedupCore {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        vector<leveldb::DB> _dbSet;
        // any additional info
    public:
        DedupCore();
        ~DedupCore();
        bool insertMQ(); 
        bool extractMQ(); 
        virtual bool dataDedup() = 0;
        // any additional functions
};