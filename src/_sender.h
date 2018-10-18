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

using namespace std;

class Sender {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _inputMQ;
        // any additional info
    public:
        Sender();
        ~Sender();
        bool extractMQ(); 
        //Implemented in a derived class and implements different types of transmissions by overloading the function
        virtual bool sendData() = 0; 
        // any additional functions
};