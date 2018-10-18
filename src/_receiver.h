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

class Receiver {
    friend class Configure;
    friend class Chunk;
    private:
        MessageQueue _outputMQ;
        // any additional info
    public:
        Receiver();
        ~Receiver();
        bool insertMQ(); 
        //Implemented in a derived class and implements different types of transmissions by overloading the function
        virtual bool receiveData() = 0; 
        // any additional functions
};