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
        while(!this->extractMQ(tmpChunk));
        file[tmpChunk.getID()]=tmpChunk.getLogicData();
    }
    for(i=0;i<_chunkCnt;i++){
        _retrieveFile.write(&file[i][0],file[i].length());
    }
    _retrieveFile.close();
    std::cerr<<"Retrieve : retrieve done\n";
}

void Retriever::run() {
    while(!this->extractMQ(_chunkCnt));
    this->Retrieve();
}