#ifndef GENERALDEDUPSYSTEM_RETRIEVER_HPP
#define GENERALDEDUPSYSTEM_RETRIEVER_HPP

#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include <bits/stdc++.h>

using namespace std;

class Retriever {
private:
    int chunkCnt_;
    std::ofstream retrieveFile_;
    virtual bool Retrieve() = 0;
    bool extractMQ(Chunk_t& chunk);

public:
    Retriever(string fileName);
    ~Retriever();
    void run();
    bool Retrieve();
};

#endif //GENERALDEDUPSYSTEM_RETRIEVER_HPP
