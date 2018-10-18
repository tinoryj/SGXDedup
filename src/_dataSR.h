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

class DataSR {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        // any additional info
    public:
        DataSR();
        ~DataSR();
        virtual bool receiveData() = 0; 
        virtual bool sendData() = 0; 
        bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertMQ & extractMQ threads).
        bool insertMQ(); 
        bool extractMQ(); 
        // any additional functions
};