//
// Created by a on 11/17/18.
//

#include "chunker.hpp"

extern Configure config;

Chunker::Chunker(std::string path, keyClient* keyClientObjTemp)
{
    loadChunkFile(path);
    ChunkerInit(path);
    cryptoObj = new CryptoPrimitive();
    keyClientObj = keyClientObjTemp;
}

std::ifstream& Chunker::getChunkingFile()
{

    if (!chunkingFile.is_open()) {
        std::cerr << "chunking file open failed" << endl;
        exit(1);
    }
    return chunkingFile;
}

void Chunker::loadChunkFile(std::string path)
{
    if (chunkingFile.is_open()) {
        chunkingFile.close();
    }
    chunkingFile.open(path, std::ios::binary);
    if (!chunkingFile.is_open()) {
        cerr << "Chunker open file: " << path << "error" << endl;
        exit(1);
    }
}

Chunker::~Chunker()
{
    if (powerLUT != NULL) {
        delete powerLUT;
    }
    if (removeLUT != NULL) {
        delete removeLUT;
    }
    if (waitingForChunkingBuffer != NULL) {
        delete waitingForChunkingBuffer;
    }
    if (chunkBuffer != NULL) {
        delete chunkBuffer;
    }
    if (cryptoObj != NULL) {
        delete cryptoObj;
    }
    if (chunkingFile.is_open()) {
        chunkingFile.close();
    }
}

void Chunker::ChunkerInit(string path)
{
    string filePathHash;
    cryptoObj->sha256_digest(path, filePathHash);
    memcpy(recipe.fileRecipeHead.fileNameHash, filePathHash.c_str(), FILE_NAME_HASH_SIZE);
    memcpy(recipe.keyRecipeHead.fileNameHash, filePathHash.c_str(), FILE_NAME_HASH_SIZE);
    ifstream fin(path, ifstream::binary);
    recipe.fileRecipeHead.fileSize = fin.tellg();
    recipe.fileRecipeHead.fileSize = recipe.fileRecipeHead.fileSize;
    fin.close();

    ChunkerType = (int)config.getChunkingType();
    // init recipe list vector size avoid too much memory malloc
    if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {
        keyRecipeList.resize(recipe.fileRecipeHead.fileSize / AVG_CHUNK_SIZE + 1);
        fileRecipeList.resize(recipe.fileRecipeHead.fileSize / AVG_CHUNK_SIZE + 1);
    } else if (ChunkerType == CHUNKER_VAR_SIZE_TYPE) {
        keyRecipeList.resize(recipe.fileRecipeHead.fileSize / MIN_CHUNK_SIZE);
        fileRecipeList.resize(recipe.fileRecipeHead.fileSize / MIN_CHUNK_SIZE);
    }

    if (ChunkerType == CHUNKER_VAR_SIZE_TYPE) {
        int numOfMaskBits;
        avgChunkSize = (int)config.getAverageChunkSize();
        minChunkSize = (int)config.getMinChunkSize();
        maxChunkSize = (int)config.getMaxChunkSize();
        slidingWinSize = (int)config.getSlidingWinSize();
        ReadSize = config.getReadSize();
        ReadSize = ReadSize * 1024 * 1024;
        waitingForChunkingBuffer = new u_char[ReadSize];
        chunkBuffer = new u_char[maxChunkSize];

        if (waitingForChunkingBuffer == NULL || chunkBuffer == NULL) {
            cerr << "Memory Error" << endl;
            exit(1);
        }
        if (minChunkSize >= avgChunkSize) {
            cerr << "Error: minChunkSize should be smaller than avgChunkSize!" << endl;
            exit(1);
        }
        if (maxChunkSize <= avgChunkSize) {
            cerr << "Error: maxChunkSize should be larger than avgChunkSize!" << endl;
            exit(1);
        }

        /*initialize the base and modulus for calculating the fingerprint of a window*/
        /*these two values were employed in open-vcdiff: "http://code.google.com/p/open-vcdiff/"*/
        polyBase = 257; /*a prime larger than 255, the max value of "unsigned char"*/
        polyMOD = (1 << 23) - 1; /*polyMOD - 1 = 0x7fffff: use the last 23 bits of a polynomial as its hash*/
        /*initialize the lookup table for accelerating the power calculation in rolling hash*/
        powerLUT = (uint32_t*)malloc(sizeof(uint32_t) * slidingWinSize);
        /*powerLUT[i] = power(polyBase, i) mod polyMOD*/
        powerLUT[0] = 1;
        for (int i = 1; i < slidingWinSize; i++) {
            /*powerLUT[i] = (powerLUT[i-1] * polyBase) mod polyMOD*/
            powerLUT[i] = (powerLUT[i - 1] * polyBase) & polyMOD;
        }
        /*initialize the lookup table for accelerating the byte remove in rolling hash*/
        removeLUT = (uint32_t*)malloc(sizeof(uint32_t) * 256); /*256 for unsigned char*/
        for (int i = 0; i < 256; i++) {
            /*removeLUT[i] = (- i * powerLUT[_slidingWinSize-1]) mod polyMOD*/
            removeLUT[i] = (i * powerLUT[slidingWinSize - 1]) & polyMOD;
            if (removeLUT[i] != 0) {

                removeLUT[i] = (polyMOD - removeLUT[i] + 1) & polyMOD;
            }
            /*note: % is a remainder (rather than modulus) operator*/
            /*      if a < 0,  -polyMOD < a % polyMOD <= 0       */
        }

        /*initialize the anchorMask for depolytermining an anchor*/
        /*note: power(2, numOfanchorMaskBits) = avgChunkSize*/
        numOfMaskBits = 1;
        while ((avgChunkSize >> numOfMaskBits) != 1) {

            numOfMaskBits++;
        }
        anchorMask = (1 << numOfMaskBits) - 1;
        /*initialize the value for depolytermining an anchor*/
        anchorValue = 0;
        cerr << "A variable size Chunker has been constructed!" << endl;
        cerr << "Parameters: " << endl;
        cerr << setw(6) << "avgChunkSize: " << avgChunkSize << endl;
        cerr << setw(6) << "minChunkSize: " << minChunkSize << endl;
        cerr << setw(6) << "maxChunkSize: " << maxChunkSize << endl;
        cerr << setw(6) << "slidingWinSize: " << slidingWinSize << endl;
        cerr << setw(6) << "polyBase: 0x" << hex << polyBase << endl;
        cerr << setw(6) << "polyMOD: 0x" << hex << polyMOD << endl;
        cerr << setw(6) << "_anchoranchorMask: 0x" << hex << anchorMask << endl;
        cerr << setw(6) << "anchorValue: 0x" << hex << anchorValue << endl;
        cerr << endl;
    } else if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {

        avgChunkSize = (int)config.getAverageChunkSize();
        minChunkSize = (int)config.getMinChunkSize();
        maxChunkSize = (int)config.getMaxChunkSize();
        ReadSize = config.getReadSize();
        ReadSize = ReadSize * 1024 * 1024;
        waitingForChunkingBuffer = new u_char[ReadSize];
        chunkBuffer = new u_char[avgChunkSize];

        if (waitingForChunkingBuffer == NULL || chunkBuffer == NULL) {
            cerr << "Memory Error" << endl;
            exit(1);
        }
        if (minChunkSize != avgChunkSize || maxChunkSize != avgChunkSize) {
            cerr << "Error: minChunkSize and maxChunkSize should be same in fixed size mode!" << endl;
            exit(1);
        }
        if (ReadSize % avgChunkSize != 0) {
            cerr << "Setting fixed size chunking error : ReadSize not compat with average chunk size" << endl;
        }
        cerr << "A fixed size Chunker has been constructed!" << endl;
        cerr << "Parameters: " << endl;
        cerr << setw(6) << "ChunkSize: " << avgChunkSize << endl;
        cerr << endl;
    }
}

bool Chunker::chunking()
{
    /*fixed-size Chunker*/
    if (config.getChunkingType() == CHUNKER_FIX_SIZE_TYPE) {
        fixSizeChunking();
    }
    /*variable-size Chunker*/
    if (config.getChunkingType() == CHUNKER_VAR_SIZE_TYPE) {
        varSizeChunking();
    }
}

void Chunker::fixSizeChunking()
{

    std::ifstream& fin = getChunkingFile();
    uint64_t chunkIDCounter = 0;

    memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);

    /*start chunking*/
    while (true) {
        fin.read((char*)waitingForChunkingBuffer, sizeof(char) * ReadSize);
        uint64_t totalReadSize = fin.gcount();
        uint64_t chunkedSize = 0;
        if (totalReadSize == ReadSize) {
            while (chunkedSize < totalReadSize) {
                memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
                memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, avgChunkSize);
                string chunkLogicData((char*)chunkBuffer, avgChunkSize), hash;
                cryptoObj->sha256_digest(chunkLogicData, hash);
                if (hash.length() != CHUNK_HASH_SIZE) {
                    cerr << "fixed size chunking: compute hash error" << endl;
                }
                Chunk_t tempChunk;
                tempChunk.ID = chunkIDCounter;
                tempChunk.logicDataSize = avgChunkSize;
                memcpy(tempChunk.logicData, chunkBuffer, avgChunkSize);
                memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
                tempChunk.type = CHUNK_TYPE_INIT;
                insertMQToKeyClient(tempChunk);

                FileRecipeEntry_t newFileRecipeEntry;
                KeyRecipeEntry_t newKeyRecipeEntry;
                newFileRecipeEntry.chunkID = tempChunk.ID;
                newKeyRecipeEntry.chunkID = tempChunk.ID;
                newFileRecipeEntry.chunkSize = tempChunk.logicDataSize;
                keyRecipeList.push_back(newKeyRecipeEntry);
                fileRecipeList.push_back(newFileRecipeEntry);

                chunkIDCounter++;
                chunkedSize += avgChunkSize;
            }
        } else {
            uint64_t retSize = 0;
            while (chunkedSize < totalReadSize) {
                memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
                if (retSize > avgChunkSize) {
                    memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, avgChunkSize);
                } else {
                    memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, retSize);
                }
                retSize = totalReadSize - chunkedSize;
                string chunkLogicData((char*)chunkBuffer, avgChunkSize), hash;
                cryptoObj->sha256_digest(chunkLogicData, hash);
                if (hash.length() != CHUNK_HASH_SIZE) {
                    cerr << "fixed size chunking: compute hash error" << endl;
                }
                Chunk_t tempChunk;
                tempChunk.ID = chunkIDCounter;
                tempChunk.logicDataSize = avgChunkSize;
                memcpy(tempChunk.logicData, chunkBuffer, avgChunkSize);
                memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
                tempChunk.type = CHUNK_TYPE_INIT;
                insertMQToKeyClient(tempChunk);

                FileRecipeEntry_t newFileRecipeEntry;
                KeyRecipeEntry_t newKeyRecipeEntry;
                newFileRecipeEntry.chunkID = tempChunk.ID;
                newKeyRecipeEntry.chunkID = tempChunk.ID;
                newFileRecipeEntry.chunkSize = tempChunk.logicDataSize;
                keyRecipeList.push_back(newKeyRecipeEntry);
                fileRecipeList.push_back(newFileRecipeEntry);

                chunkIDCounter++;
                chunkedSize += avgChunkSize;
            }
        }
        if (fin.eof()) {
            break;
        }
    }
    recipe.fileRecipeHead.totalChunkNumber = chunkIDCounter;
    recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCounter;
    if (setJobDoneFlag() == false) {
        cerr << "Chunker: set chunking done flag error" << endl;
    }
    cout << "Fixed chunking over: Total file size = " << recipe.fileRecipeHead.fileSize << " ; Total chunk number = " << chunkIDCounter << endl;
}

void Chunker::varSizeChunking()
{
    uint16_t winFp;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = 0;
    ifstream& fin = getChunkingFile();

    /*start chunking*/
    while (true) {
        fin.read((char*)waitingForChunkingBuffer, sizeof(unsigned char) * ReadSize);
        int len = fin.gcount();
        for (int i = 0; i < len; i++) {

            chunkBuffer[chunkBufferCnt] = waitingForChunkingBuffer[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize) {
                winFp = winFp + (chunkBuffer[chunkBufferCnt] * powerLUT[slidingWinSize - chunkBufferCnt - 1]) & polyMOD; //Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (polyMOD);

            /*slide window*/
            unsigned short int v = chunkBuffer[chunkBufferCnt - slidingWinSize]; //queue
            winFp = ((winFp + removeLUT[v]) * polyBase + chunkBuffer[chunkBufferCnt]) & polyMOD; //remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize*/
            if (chunkBufferCnt < minChunkSize)
                continue;

            /*find chunk pattern*/
            if ((winFp & anchorMask) == anchorValue) {

                string chunkLogicData((char*)chunkBuffer, chunkBufferCnt), hash;
                cryptoObj->sha256_digest(chunkLogicData, hash);
                Chunk_t tempChunk;
                tempChunk.ID = chunkIDCnt;
                tempChunk.logicDataSize = chunkBufferCnt;
                memcpy(tempChunk.logicData, chunkLogicData.c_str(), chunkLogicData.length());
                memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
                tempChunk.type = CHUNK_TYPE_INIT;
                insertMQToKeyClient(tempChunk);

                FileRecipeEntry_t newFileRecipeEntry;
                KeyRecipeEntry_t newKeyRecipeEntry;
                newFileRecipeEntry.chunkID = tempChunk.ID;
                newKeyRecipeEntry.chunkID = tempChunk.ID;
                newFileRecipeEntry.chunkSize = tempChunk.logicDataSize;
                keyRecipeList.push_back(newKeyRecipeEntry);
                fileRecipeList.push_back(newFileRecipeEntry);

                chunkIDCnt++;
                chunkBufferCnt = winFp = 0;
                totalSize += chunkLogicData.length();
            }

            /*chunk's size exceed maxChunkSize*/
            if (chunkBufferCnt >= maxChunkSize) {

                string chunkLogicData((char*)chunkBuffer, chunkBufferCnt), hash;
                cryptoObj->sha256_digest(chunkLogicData, hash);
                Chunk_t tempChunk;
                tempChunk.ID = chunkIDCnt;
                tempChunk.logicDataSize = chunkBufferCnt;
                memcpy(tempChunk.logicData, chunkLogicData.c_str(), chunkLogicData.length());
                memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
                tempChunk.type = CHUNK_TYPE_INIT;
                insertMQToKeyClient(tempChunk);

                FileRecipeEntry_t newFileRecipeEntry;
                KeyRecipeEntry_t newKeyRecipeEntry;
                newFileRecipeEntry.chunkID = tempChunk.ID;
                newKeyRecipeEntry.chunkID = tempChunk.ID;
                newFileRecipeEntry.chunkSize = tempChunk.logicDataSize;
                keyRecipeList.push_back(newKeyRecipeEntry);
                fileRecipeList.push_back(newFileRecipeEntry);

                chunkIDCnt++;
                chunkBufferCnt = winFp = 0;
                totalSize += chunkLogicData.length();
            }
        }
        if (fin.eof())
            break;
    }

    /*add final chunk*/
    if (chunkBufferCnt != 0) {

        string chunkLogicData((char*)chunkBuffer, chunkBufferCnt), hash;
        cryptoObj->sha256_digest(chunkLogicData, hash);
        Chunk_t tempChunk;
        tempChunk.ID = chunkIDCnt;
        tempChunk.logicDataSize = chunkBufferCnt;
        memcpy(tempChunk.logicData, chunkLogicData.c_str(), chunkLogicData.length());
        memcpy(tempChunk.chunkHash, hash.c_str(), CHUNK_HASH_SIZE);
        tempChunk.type = CHUNK_TYPE_INIT;
        insertMQToKeyClient(tempChunk);
        chunkIDCnt++;
        chunkBufferCnt = winFp = 0;
        totalSize += chunkLogicData.length();
    }
    recipe.fileRecipeHead.totalChunkNumber = chunkIDCnt;
    recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCnt;
    if (setJobDoneFlag() == false) {
        cerr << "Chunker: set chunking done flag error" << endl;
    }
    cout << "Fixed chunking over: Total file size = " << recipe.fileRecipeHead.fileSize << " ; Total chunk number = " << chunkIDCounter << endl;
}

FileRecipeList_t Chunker::getFileRecipeList()
{
    return fileRecipeList;
}

KeyRecipeList_t Chunker::getKeyRecipeList()
{
    return keyRecipeList;
}

Recipe_t Chunker::getRecipeHead()
{
    return recipe;
}

bool Chunker::insertMQToKeyClient(Chunk_t newChunk)
{
    return keyClientObj->insertMQFromChunker(newChunk);
}

bool Chunker::setJobDoneFlag()
{
    return keyClientObj->editJobDoneFlag();
}