#ifndef SGXDEDUP__CHUNKER_HPP
#define SGXDEDUP__CHUNKER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#if FINGERPRINTER_MODULE_ENABLE == 1
#include "fingerprinter.hpp"
#else
#include "keyClient.hpp"
#endif
#include "messageQueue.hpp"

class Chunker {
private:
    CryptoPrimitive* cryptoObj_;
#if FINGERPRINTER_MODULE_ENABLE == 1
    Fingerprinter* FingerprinterObj_;
#else
    KeyClient* keyClientObj_;
#endif
    // Chunker type setting (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
    int ChunkerType_;
    /*chunk size setting*/
    uint32_t avgChunkSize_;
    uint32_t minChunkSize_;
    uint32_t maxChunkSize_;

    u_char *waitingForChunkingBuffer_, *chunkBuffer_;
    uint64_t ReadSize_;
    uint64_t totalSize_;
    Data_t fileRecipe_;
    std::ifstream chunkingFile_;

    /*VarSize chunking*/
    /*sliding window size*/
    uint32_t slidingWinSize_;
    uint32_t polyBase_;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint32_t polyMOD_;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint32_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint32_t* powerLUT_;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint32_t* removeLUT_;
    /*the mask for determining an anchor*/
    uint32_t anchorMask_;
    /*the value for determining an anchor*/
    uint32_t anchorValue_;

    void fixSizeChunking();
    void varSizeChunking();
    void traceDrivenChunkingFSL();
    void traceDrivenChunkingUBC();
    void ChunkerInit(string path);
    void loadChunkFile(string path);
    std::ifstream& getChunkingFile();

public:
#if FINGERPRINTER_MODULE_ENABLE == 1
    Chunker(std::string path, Fingerprinter* FingerprinterObjTemp);
#else
    Chunker(std::string path, KeyClient* keyClientObjTemp);
#endif
    ~Chunker();
    bool chunking();
    Recipe_t getRecipeHead();
};

#endif //SGXDEDUP_CHUNKER_HPP
