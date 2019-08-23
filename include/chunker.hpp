#ifndef GENERALDEDUPSYSTEM__CHUNKER_HPP
#define GENERALDEDUPSYSTEM__CHUNKER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "keyClient.hpp"
#include "messageQueue.hpp"

class Chunker {
private:
    CryptoPrimitive* cryptoObj;
    keyClient* keyClientObj;

    //Chunker type setting (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
    int ChunkerType;
    //chunk size setting
    int avgChunkSize;
    int minChunkSize;
    int maxChunkSize;
    //sliding window size
    int slidingWinSize;

    u_char *waitingForChunkingBuffer, *chunkBuffer;
    uint64_t ReadSize;

    uint32_t polyBase;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint32_t polyMOD;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint32_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint32_t* powerLUT;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint32_t* removeLUT;
    /*the mask for determining an anchor*/
    uint32_t anchorMask;
    /*the value for determining an anchor*/
    uint32_t anchorValue;

    uint64_t totalSize;

    Data_t recipe;
    std::ifstream chunkingFile;

    void fixSizeChunking();

    void varSizeChunking();

    void ChunkerInit(string path);

    bool insertMQToKeyClient(Data_t& newData);

    bool setJobDoneFlag();

    void loadChunkFile(string path);

    std::ifstream& getChunkingFile();

public:
    Chunker(std::string path, keyClient* keyClientObjTemp);
    ~Chunker();
    bool chunking();
    Recipe_t getRecipeHead();
};

#endif //GENERALDEDUPSYSTEM_CHUNKER_HPP
