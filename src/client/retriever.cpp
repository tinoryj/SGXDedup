#include "retriever.hpp"
#include "../pow/include/hexutil.h"

Retriever::Retriever(string fileName)
{
    retrieveFile_.open(fileName, ofstream::out | ofstream::binary);
}

bool Retriever::Retrieve()
{
    vector<string> file;
    file.resize(chunkCnt_);
    int i;
    Chunk_t tmpChunk;
    for (i = 0; i < chunkCnt_; i++) {
        while (!this->extractMQ(tmpChunk))
            ;
        file[tmpChunk.getID()] = tmpChunk.getLogicData();
    }
    for (i = 0; i < chunkCnt_; i++) {
        retrieveFile_.write(file[i].c_str(), file[i].length());
        const char* s = hexstring(&file[i][0], file[i].length());
        cout << "ID : " << i << endl;
        cout << s << endl
             << endl;
    }
    retrieveFile_.close();
    std::cerr << "Retrieve : retrieve done" << endl;
}

void Retriever::run()
{
    while (!this->extractMQ(chunkCnt_))
        ;
    this->Retrieve();
}