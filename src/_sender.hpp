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

class _Sender {
    private:
        MessageQueue<Chunk> _inputMQ;
        // any additional info
    public:
        _Sender();
        ~_Sender();
        bool extractMQ(); 
        //Implemented in a derived class and implements different types of transmissions by overloading the function
        virtual bool sendData() = 0; 
        MessageQueue<Chunk> getInputMQ();
        // any additional functions
};