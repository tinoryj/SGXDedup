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

class _Receiver {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _outputMQ;
        // any additional info
    public:
        _Receiver();
        ~_Receiver();
        bool insertMQ(); 
        //Implemented in a derived class and implements different types of transmissions by overloading the function
        virtual bool receiveData() = 0; 
        MessageQueue getOutputMQ();
        // any additional functions
};