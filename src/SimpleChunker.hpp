#ifndef SIMPLECHUNKER_HPP
#define SIMPLECHUNKER_HPP

#include "chunk.hpp"
#include "_chunker.hpp"

#include <queue>

extern Configure config;
//extern MessageQueue<Chunk>mq1;

class SimpleChunker:public _Chunker{
private:
    void varSizeChunking();

    //void fixSizeChunking();

public:
    SimpleChunker();
    
    SimpleChunker(std::string path);
    
    bool chunking();
};

SimpleChunker::SimpleChunker(){}

SimpleChunker::SimpleChunker(std::string path):_Chunker(path){}

bool SimpleChunker::chunking(){    
    /*fixed-size chunker*/
    if (config.getChunkingType() == FIX_SIZE_TYPE) {

        //fixSizeChunking();
    }
    /*variable-size chunker*/
    if (config.getChunkingType() == VAR_SIZE_TYPE) {

        varSizeChunking();
    }
}

void SimpleChunker::varSizeChunking() {
    using namespace std;

#ifdef DEBUG
    double tot = 0, cnt = 0;
#endif

    unsigned short int mask = 0xff, magic = 0x2B;
    unsigned short int winFp;
    unsigned char *readBuffer, *chunkBuffer;
    unsigned long long chunkBufferCnt = 0, chunkIDCnt = 0;
    unsigned long long maxChunkSize = config.getMaxChunkSize();
    unsigned long long minChunkSize = config.getMinChunkSize();
    unsigned long long slidingWinSize = config.getSlidingWinSize();
    unsigned long long ReadSize  = config.getReadSize();

    ReadSize = ReadSize * 1024 * 1024 / 8;

    readBuffer = new unsigned char[ReadSize];
    chunkBuffer = new unsigned char[maxChunkSize];
    if (readBuffer == NULL || chunkBuffer == NULL){
        cerr << "Memory Error\n" << endl;
        exit(1);
    }


    Chunk *tmpchunk;
    std::ifstream &fin = getChunkingFile();
    int rig=0;

    while (1) {
        fin.read((char *) readBuffer, sizeof(unsigned char) * ReadSize);              //read ReadSize byte from file every times
        int len, i;
        len = fin.gcount();
        for (i = 0; i < len; i++) {
            rig++;

            /*full fill sildingwindow*/
            winFp ^= readBuffer[i];
            chunkBuffer[chunkBufferCnt++] = readBuffer[i];
            if (chunkBufferCnt < slidingWinSize)continue;
            
            /*slide window*/
            unsigned short int v = chunkBuffer[chunkBufferCnt - slidingWinSize];//queue
            winFp ^= v;

            /*chunk's size less than minchunksize*/
            if (chunkBufferCnt < minChunkSize)continue;

            /*find chunk pattern*/
            if ((winFp & mask) == magic) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//        cout<<"chunk "<<chunkIDCnt<<" : "<<chunkBufferCnt<<endl;
                tot += chunkBufferCnt;
                cnt += 1;
#endif

                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
                continue;
            }

            /*chunk's size exceed maxchunksize*/
            if (chunkBufferCnt >= maxChunkSize) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) chunkBuffer);

#ifdef DEBUG
//        cout<<"chunk "<<chunkIDCnt<<" : "<<chunkBufferCnt<<endl;
                tot += chunkBufferCnt;
                cnt += 1;
#endif

                chunkBufferCnt = winFp = 0;
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
//        cout<<"chunk "<<chunkIDCnt<<" : "<<chunkBufferCnt<<endl;
        tot += chunkBufferCnt;
        cnt += 1;
#endif

        chunkBufferCnt = winFp = 0;
        //mq1.push(*tmpchunk);
        delete tmpchunk;
    }
#ifdef DEBUG
    cout << tot / cnt << endl;
#endif

    delete readBuffer;
    delete chunkBuffer;
}

#endif