//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM__CHUNKER_HPP
#define GENERALDEDUPSYSTEM__CHUNKER_HPP

#include <iostream>
#include "_messageQueue.hpp"
#include "chunk.hpp"
#include "CryptoPrimitive.hpp"
#include "_chunker.hpp"

#define FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker
#define SIMPLE_CHUNKING 2

class chunker : public _Chunker {
private:

    _messageQueue _outputMq;
    CryptoPrimitive *_cryptoObj;

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

    bool insertMQ(Chunk newChunk);

public:
    bool chunking();

    ~chunker();

    chunker();

    chunker(std::string path);

};


#endif //GENERALDEDUPSYSTEM_CHUNKER_HPP
