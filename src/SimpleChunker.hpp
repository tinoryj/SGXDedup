#ifndef SIMPLECHUNKER_HPP
#define SIMPLECHUNKER_HPP

#define DEBUG

#include "chunk.hpp"
#include "_chunker.hpp"

#include <queue>

extern Configure config;
//extern MessageQueue<Chunk>mq1;

class SimpleChunker:public _Chunker{
public:
    SimpleChunker();
    SimpleChunker(std::string path);
    bool chunking();
};


SimpleChunker::SimpleChunker(){}
SimpleChunker::SimpleChunker(std::string path):_Chunker(path){}

bool SimpleChunker::chunking() {
#ifdef DEBUG
    using namespace std;
    double tot = 0, cnt = 0;
#endif

    unsigned short int mask = 0x7ff, magic = 0x2B;
    unsigned short int hashval;
    unsigned char readBuffer[128], *chunkBuffer;
    unsigned long long chunkBufferCnt = 0, chunkIDCnt = 0;
    unsigned long long maxChunkSize = config.getMaxChunkSize();
    unsigned long long minChunkSize = config.getMinChunkSize();
    unsigned long long slidingWinSize = config.getSlidingWinSize();

    chunkBuffer = new unsigned char(maxChunkSize);
    Chunk *tmpchunk;
    std::ifstream &fin = getChunkingFile();

    while (1) {
        fin.read((char *) readBuffer, sizeof(readBuffer));              //read 128 byte from file
        int len, i;
        len = fin.gcount();
        for (i = 0; i < len; i++) {

            hashval ^= readBuffer[i];
            chunkBuffer[chunkBufferCnt++] = readBuffer[i];

            if (chunkBufferCnt < slidingWinSize)continue;       //if the window not full, don't need to add chunk
            //else slide window
            unsigned short int v = chunkBuffer[chunkBufferCnt - slidingWinSize];
            hashval ^= v;
            if (chunkBufferCnt < minChunkSize)continue;       //this chunk is small than minum chunk size

            if ((hashval & mask) == magic) {                     //find a chunk bound
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//              cout<<chunkBufferCnt<<endl;
                tot += chunkBufferCnt;
                cnt += 1;
#endif

                chunkBufferCnt = hashval = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
                continue;
            }
            if (chunkBufferCnt >= maxChunkSize) {                //this chunk is bigger than maxnum chunk size
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//                  cout<<chunkBufferCnt<<endl;
                tot += chunkBufferCnt;
                cnt += 1;
#endif

                chunkBufferCnt = hashval = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
            }
        }
        if (fin.eof())break;
    }

    //add final chunk
    if (!chunkBufferCnt) {
        tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//          cout<<chunkBufferCnt<<endl;
        tot += chunkBufferCnt;
        cnt += 1;
#endif

        chunkBufferCnt = hashval = 0;
        //mq1.push(*tmpchunk);
        delete tmpchunk;
    }
#ifdef DEBUG
    cout << tot / cnt << endl;
#endif
}

#endif