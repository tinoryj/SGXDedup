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

class _DataSR {
    private:
        MessageQueue _inputMQ;
        MessageQueue _outputMQ;
        // any additional info
    public:
        _DataSR();
        ~_DataSR();
        virtual bool receiveData() = 0; 
        virtual bool sendData() = 0; 
        bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertMQ & extractMQ threads).
        bool insertMQ(); 
        bool extractMQ(); 
        MessageQueue getInputMQ();
        MessageQueue getOutputMQ();
        // any additional functions
};