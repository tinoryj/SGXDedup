//
// Created by a on 1/30/19.
//

#ifndef GENERALDEDUPSYSTEM_RETRIEVER_HPP
#define GENERALDEDUPSYSTEM_RETRIEVER_HPP

#include "_retriever.hpp"

class Retriever:public _Retriever{
private:
    int _chunkCnt;
public:
    Retriever(string filename);
    void run();
    bool Retrieve();
};

#endif //GENERALDEDUPSYSTEM_RETRIEVER_HPP
