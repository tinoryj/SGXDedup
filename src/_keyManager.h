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

class KeyManager {
    friend class Configure;
    friend class Chunk;
    private:
        deque<Chunk> receiveQue;
        deque<Chunk> sendQue;
        // any additional info
    public:
        KeyManager();
        ~KeyManager();
        virtual bool receiveData() = 0; 
        virtual bool sendData() = 0; 
        virtual bool keyGen() = 0;
        bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertQue & extractQue threads).
        bool insertQue(); 
        bool extractQue(); 
        // any additional functions
};