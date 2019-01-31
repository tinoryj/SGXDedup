//
// Created by a on 1/30/19.
//

#include "retriever.hpp"


Retriever::Retriever(string filename):_Retriever(filename) {}

bool Retriever::Retrieve() {
    vector<string>file;
    file.resize(_chunkCnt);
    int i;
    Chunk tmpChunk;
    for(i=0;i<_chunkCnt;i++){
        this->extractMQ(tmpChunk);
        file[tmpChunk.getID()]=tmpChunk.getLogicDataSize();
    }
    for(i=0;i<_chunkCnt;i++){
        _retrieveFile.write(&file[i][0],file[i].length());
    }
}

void Retriever::run() {
    this->extractMQ(_chunkCnt);
    this->Retrieve();
}