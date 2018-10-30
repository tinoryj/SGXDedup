#ifndef CHUNKER_HPP
#define CHUNKER_HPP

#include <iostream>
#include "chunk.hpp"
#include "_chunker.hpp"

#define FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker
#define SIMPLE_CHUNKING 2

class chunker : public _Chunker {
private:

    //chunker type setting (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
    bool _chunkerType;
    //chunk size setting
    int _avgChunkSize;
    int _minChunkSize;
    int _maxChunkSize;
    //sliding window size
    int _slidingWinSize;

    unsigned char *_waitingForChunkingBuffer, *_chunkBuffer;
    unsigned long long _ReadSize;


    uint32_t _polyBase;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint32_t _polyMOD;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint32_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint32_t *_powerLUT;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint32_t *_removeLUT;
    /*the mask for determining an anchor*/
    uint32_t _anchorMask;
    /*the value for determining an anchor*/
    uint32_t _anchorValue;

    void fixSizeChunking();

    void varSizeChunking();

    void simpleChunking();

    void chunkerInit();

public:
    bool chunking();

    ~chunker();

    chunker();

    chunker(std::string path);
};


extern Configure config;

chunker::chunker() {}

chunker::chunker(std::string path) {
    loadChunkFile(path);
    chunkerInit();
}

chunker::~chunker() {
    if (_powerLUT != NULL)delete _powerLUT;
    if (_removeLUT != NULL)delete _removeLUT;
    if (_waitingForChunkingBuffer != NULL)delete _waitingForChunkingBuffer;
    if (_chunkBuffer != NULL)delete _chunkBuffer;
}

void chunker::chunkerInit() {

    using namespace std;

    int numOfMaskBits;

    _avgChunkSize = config.getAverageChunkSize();
    _minChunkSize = config.getMinChunkSize();
    _maxChunkSize = config.getMaxChunkSize();
    _slidingWinSize = config.getSlidingWinSize();
    _ReadSize = config.getReadSize();
    _ReadSize = _ReadSize * 1024 * 1024 / 8;
    _waitingForChunkingBuffer = new unsigned char[_ReadSize];
    _chunkBuffer = new unsigned char[_maxChunkSize];

    if (_waitingForChunkingBuffer == NULL || _chunkBuffer == NULL) {
        cerr << "Memory Error\n" << endl;
        exit(1);
    }
    if (_minChunkSize >= _avgChunkSize) {
        cerr << "Error: _minChunkSize should be smaller than avgChunkSize!" << endl;
        exit(1);
    }
    if (_maxChunkSize <= _avgChunkSize) {
        cerr << "Error: _maxChunkSize should be larger than avgChunkSize!" << endl;
        exit(1);
    }

    /*initialize the base and modulus for calculating the fingerprint of a window*/
    /*these two values were employed in open-vcdiff: "http://code.google.com/p/open-vcdiff/"*/
    _polyBase = 257; /*a prime larger than 255, the max value of "unsigned char"*/
    _polyMOD = (1 << 23) - 1; /*_polyMOD - 1 = 0x7fffff: use the last 23 bits of a polynomial as its hash*/
    /*initialize the lookup table for accelerating the power calculation in rolling hash*/
    _powerLUT = (uint32_t *) malloc(sizeof(uint32_t) * _slidingWinSize);
    /*_powerLUT[i] = power(_polyBase, i) mod _polyMOD*/
    _powerLUT[0] = 1;
    for (int i = 1; i < _slidingWinSize; i++) {
        /*_powerLUT[i] = (_powerLUT[i-1] * _polyBase) mod _polyMOD*/
        _powerLUT[i] = (_powerLUT[i - 1] * _polyBase) & _polyMOD;
    }
    /*initialize the lookup table for accelerating the byte remove in rolling hash*/
    _removeLUT = (uint32_t *) malloc(sizeof(uint32_t) * 256); /*256 for unsigned char*/
    for (int i = 0; i < 256; i++) {
        /*_removeLUT[i] = (- i * _powerLUT[__slidingWinSize-1]) mod _polyMOD*/
        _removeLUT[i] = (i * _powerLUT[_slidingWinSize - 1]) & _polyMOD;
        if (_removeLUT[i] != 0) {

            _removeLUT[i] = (_polyMOD - _removeLUT[i] + 1) & _polyMOD;
        }
        /*note: % is a remainder (rather than modulus) operator*/
        /*      if a < 0,  -_polyMOD < a % _polyMOD <= 0       */
    }

    /*initialize the _anchorMask for depolytermining an anchor*/
    /*note: power(2, numOf_anchorMaskBits) = _avgChunkSize*/
    numOfMaskBits = 1;
    while ((_avgChunkSize >> numOfMaskBits) != 1) {

        numOfMaskBits++;
    }
    _anchorMask = (1 << numOfMaskBits) - 1;
    /*initialize the value for depolytermining an anchor*/
    _anchorValue = 0;
    cerr << endl << "A variable-size chunker has been constructed!" << endl;
    cerr << "Parameters: " << endl;
    cerr << setw(6) << "_avgChunkSize: " << _avgChunkSize << endl;
    cerr << setw(6) << "_minChunkSize: " << _minChunkSize << endl;
    cerr << setw(6) << "_maxChunkSize: " << _maxChunkSize << endl;
    cerr << setw(6) << "_slidingWinSize: " << _slidingWinSize << endl;
    cerr << setw(6) << "_polyBase: 0x" << hex << _polyBase << endl;
    cerr << setw(6) << "_polyMOD: 0x" << hex << _polyMOD << endl;
    cerr << setw(6) << "_anchor_anchorMask: 0x" << hex << _anchorMask << endl;
    cerr << setw(6) << "_anchorValue: 0x" << hex << _anchorValue << endl;
    cerr << endl;
}

bool chunker::chunking() {
    /*fixed-size chunker*/
    if (config.getChunkingType() == FIX_SIZE_TYPE) {

        fixSizeChunking();
    }
    /*variable-size chunker*/
    if (config.getChunkingType() == VAR_SIZE_TYPE) {

        varSizeChunking();
    }
    /*simple chunker*/
    if (config.getChunkingType() == SIMPLE_CHUNKING) {

        simpleChunking();
    }
}

void chunker::fixSizeChunking() {

    Chunk *tmpchunk;
    std::ifstream &fin = getChunkingFile();
    unsigned long long chunkBufferCnt = 0, chunkIDCnt = 0;

    /*start chunking*/
    while (1) {
        fin.read((char *) _waitingForChunkingBuffer, sizeof(char) * _ReadSize);
        int len = fin.gcount(), x = 0, y;
        while (x < len) {
            y = std::min((long long) (_avgChunkSize - chunkBufferCnt), (long long) (len - x));
            memcpy(chunkBufferCnt + _chunkBuffer, _waitingForChunkingBuffer + x, sizeof(char) * y);
            x += y;
            chunkBufferCnt += y;
            if (chunkBufferCnt >= _avgChunkSize) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);
                //mq.insert(tmpchunk);
                delete tmpchunk;

#ifdef DEBUG
                std::cout<<chunkBufferCnt<<std::endl;
#endif

                chunkBufferCnt = 0;
            }
        }
        if (fin.eof())break;
    }
    if (chunkBufferCnt != 0) {
        tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);
#ifdef DEBUG
        std::cout<<chunkBufferCnt<<std::endl;
#endif

        //mq.insert(tmpchunk);
        delete tmpchunk;
    }
}

void chunker::varSizeChunking() {
    using namespace std;

    unsigned short int winFp;
    unsigned long long chunkBufferCnt = 0, chunkIDCnt = 0;
    Chunk *tmpchunk;
    ifstream &fin = getChunkingFile();


    /*start chunking*/
    while (1) {
        fin.read((char *) _waitingForChunkingBuffer, sizeof(unsigned char) * _ReadSize);
        int i, len = fin.gcount();
        for (i = 0; i < len; i++) {

            _chunkBuffer[chunkBufferCnt] = _waitingForChunkingBuffer[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < _slidingWinSize) {
                winFp = winFp + (_chunkBuffer[chunkBufferCnt] * _powerLUT[_slidingWinSize - chunkBufferCnt - 1]) &
                        _polyMOD;//Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (_polyMOD);

            /*slide window*/
            unsigned short int v = _chunkBuffer[chunkBufferCnt - _slidingWinSize];//queue
            winFp = ((winFp + _removeLUT[v]) * _polyBase + _chunkBuffer[chunkBufferCnt]) &
                    _polyMOD;//remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than _minChunkSize*/
            if (chunkBufferCnt < _minChunkSize)continue;

            /*find chunk pattern*/
            if ((winFp & _anchorMask) == _anchorValue) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);

#ifdef DEBUG
                std::cout<<chunkBufferCnt<<std::endl;
#endif

                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
            }

            /*chunk's size exceed _maxChunkSize*/
            if (chunkBufferCnt >= _maxChunkSize) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);

                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
            }
        }
        if (fin.eof())break;
    }

    /*add final chunk*/
    if (chunkBufferCnt != 0) {
        tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);

#ifdef DEBUG
        std::cout<<chunkBufferCnt<<std::endl;
#endif

        //mq.insert(tmpchunk);
        delete tmpchunk;
    }

}


void chunker::simpleChunking() {
    using namespace std;

    unsigned short int winFp;
    unsigned long long chunkBufferCnt = 0, chunkIDCnt = 0;
    Chunk *tmpchunk;
    std::ifstream &fin = getChunkingFile();

    /*start chunking*/
    while (1) {
        fin.read((char *) _waitingForChunkingBuffer,
                 sizeof(unsigned char) * _ReadSize);              //read _ReadSize byte from file every times
        int len, i;
        len = fin.gcount();
        for (i = 0; i < len; i++) {

            /*full fill sildingwindow*/
            winFp ^= _waitingForChunkingBuffer[i];
            _chunkBuffer[chunkBufferCnt++] = _waitingForChunkingBuffer[i];
            if (chunkBufferCnt < _slidingWinSize)continue;

            /*slide window*/
            unsigned short int v = _chunkBuffer[chunkBufferCnt - _slidingWinSize];//queue
            winFp ^= v;

            /*chunk's size less than _minChunkSize*/
            if (chunkBufferCnt < _minChunkSize)continue;

            /*find chunk pattern*/
            if ((winFp & _anchorMask) == _anchorValue) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);

#ifdef DEBUG
                std::cout<<chunkBufferCnt<<std::endl;
#endif

                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
                continue;
            }

            /*chunk's size exceed _maxChunkSize*/
            if (chunkBufferCnt >= _maxChunkSize) {
                tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);

#ifdef DEBUG
                std::cout<<chunkBufferCnt<<std::endl;
#endif

                chunkBufferCnt = winFp = 0;
                //mq1.push(*tmpchunk);
                delete tmpchunk;
            }
        }
        if (fin.eof())break;
    }

    //add final chunk
    if (chunkBufferCnt != 0) {
        tmpchunk = new Chunk(chunkIDCnt++, 0, chunkBufferCnt, (char *) _chunkBuffer);

#ifdef DEBUG
        std::cout<<chunkBufferCnt<<std::endl;
#endif

        //mq.insert(tmpchunk);
        delete tmpchunk;
    }
}

#endif