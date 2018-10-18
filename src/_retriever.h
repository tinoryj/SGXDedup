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

class Retriever {
    friend class Configure;
    friend class Chunk;
    private:
        ofstream _retrieveFile;
        // any additional info 
    public:
        Retriever();
        ~Retriever();
        virtual bool Retrieve() = 0;
        bool extractMQ();
        // any additional functions
};