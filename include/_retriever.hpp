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

class Retriever {
    private:
        MessageQueue _inputMQ;
        std::ofstream _retrieveFile;
        // any additional info 
    public:
        Retriever();
        ~Retriever();
        virtual bool Retrieve() = 0;
        bool extractMQ();
        MessageQueue getInputMQ();
        // any additional functions
};