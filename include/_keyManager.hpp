//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYMANAGER_HPP
#define GENERALDEDUPSYSTEM_KEYMANAGER_HPP
#include <cstdio>
#include <algorithm>
#include <iostream>
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
#include <string>

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "message.hpp"
//#include "leveldb/db.h"

class _keyManager {
private:
    _messageQueue receiveQue;
//  _messageQueue sendQue;
//    std::deque<Chunk> receiveQue;
//    std::deque<Chunk> sendQue;
    // any additional info

public:
    _keyManager();
    ~_keyManager();
    //virtual bool receiveData() = 0;
    //virtual bool sendData() = 0;
    virtual bool keyGen(std::string hash,std::string &key) = 0;
    bool workloadProgress(std::string hash,std::string &key); // main function for epoll S/R and threadPool schedule (insertQue & extractQue threads).
    bool insertQue();
    bool extractQue();
    //std::deque<Chunk> getReceiveQue();
    //std::deque<Chunk> getSendQue();
    // any additional functions
};

#endif //GENERALDEDUPSYSTEM_KEYMANAGER_HPP
