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

class _KeyManager {
    private:
        std::deque<Chunk> receiveQue;
        std::deque<Chunk> sendQue;
        // any additional info
    public:
        _KeyManager();
        ~_KeyManager();
        virtual bool receiveData() = 0; 
        virtual bool sendData() = 0; 
        virtual bool keyGen() = 0;
        bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertQue & extractQue threads).
        bool insertQue(); 
        bool extractQue(); 
        std::deque<Chunk> getReceiveQue();
        std::deque<Chunk> getSendQue();
        // any additional functions
};