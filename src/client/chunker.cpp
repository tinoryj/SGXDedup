#include "chunker.hpp"
#include "sys/time.h"
extern Configure config;

struct timeval timestartChunker;
struct timeval timeendChunker;
struct timeval timestartChunkerInsertMQ;
struct timeval timeendChunkerInsertMQ;
struct timeval timestartChunkerReadFile;
struct timeval timeendChunkerReadFile;

void PRINT_BYTE_ARRAY_CHUNKER(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

Chunker::Chunker(std::string path, keyClient* keyClientObjTemp)
{
    loadChunkFile(path);
    ChunkerInit(path);
    cryptoObj = new CryptoPrimitive();
    keyClientObj = keyClientObjTemp;
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

std::ifstream& Chunker::getChunkingFile()
{
    if (!chunkingFile.is_open()) {
        cerr << "Chunker : chunking file open failed" << endl;
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
        cerr << "Chunker : open file: " << path << "error, client exit now" << endl;
        exit(1);
    }
}

void Chunker::ChunkerInit(string path)
{
    u_char filePathHash[FILE_NAME_HASH_SIZE];
    cryptoObj->generateHash((u_char*)&path[0], path.length(), filePathHash);
    memcpy(fileRecipe.recipe.fileRecipeHead.fileNameHash, filePathHash, FILE_NAME_HASH_SIZE);
    memcpy(fileRecipe.recipe.keyRecipeHead.fileNameHash, filePathHash, FILE_NAME_HASH_SIZE);

    ChunkerType = (int)config.getChunkingType();

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
            cerr << "Chunker : Memory malloc error" << endl;
            exit(1);
        }
        if (minChunkSize >= avgChunkSize) {
            cerr << "Chunker : minChunkSize should be smaller than avgChunkSize!" << endl;
            exit(1);
        }
        if (maxChunkSize <= avgChunkSize) {
            cerr << "Chunker : maxChunkSize should be larger than avgChunkSize!" << endl;
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
    } else if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {

        avgChunkSize = (int)config.getAverageChunkSize();
        minChunkSize = (int)config.getMinChunkSize();
        maxChunkSize = (int)config.getMaxChunkSize();
        ReadSize = config.getReadSize();
        ReadSize = ReadSize * 1024 * 1024;
        waitingForChunkingBuffer = new u_char[ReadSize];
        chunkBuffer = new u_char[avgChunkSize];

        if (waitingForChunkingBuffer == NULL || chunkBuffer == NULL) {
            cerr << "Chunker : Memory Error" << endl;
            exit(1);
        }
        if (minChunkSize != avgChunkSize || maxChunkSize != avgChunkSize) {
            cerr << "Chunker : Error: minChunkSize and maxChunkSize should be same in fixed size mode!" << endl;
            exit(1);
        }
        if (ReadSize % avgChunkSize != 0) {
            cerr << "Chunker : Setting fixed size chunking error : ReadSize not compat with average chunk size" << endl;
        }
    } else if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {

        avgChunkSize = (int)config.getAverageChunkSize();
        minChunkSize = (int)config.getMinChunkSize();
        maxChunkSize = (int)config.getMaxChunkSize();
        ReadSize = config.getReadSize();
        ReadSize = ReadSize * 1024 * 1024;
        waitingForChunkingBuffer = new u_char[ReadSize];
        chunkBuffer = new u_char[avgChunkSize];

        if (waitingForChunkingBuffer == NULL || chunkBuffer == NULL) {
            cerr << "Chunker : Memory Error" << endl;
            exit(1);
        }
        if (minChunkSize != avgChunkSize || maxChunkSize != avgChunkSize) {
            cerr << "Chunker : Error: minChunkSize and maxChunkSize should be same in fixed size mode!" << endl;
            exit(1);
        }
        if (ReadSize % avgChunkSize != 0) {
            cerr << "Chunker : Setting fixed size chunking error : ReadSize not compat with average chunk size" << endl;
        }
    } else if (ChunkerType == CHUNKER_TRACE_DRIVEN_TYPE_FSL) {
        maxChunkSize = (int)config.getMaxChunkSize();
        chunkBuffer = new u_char[maxChunkSize + 6];
    } else if (ChunkerType == CHUNKER_TRACE_DRIVEN_TYPE_UBC) {
        maxChunkSize = (int)config.getMaxChunkSize();
        chunkBuffer = new u_char[maxChunkSize + 5];
    }
}

bool Chunker::chunking()
{
    /*fixed-size Chunker*/
    if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {
        fixSizeChunking();
    }
    /*variable-size Chunker*/
    if (ChunkerType == CHUNKER_VAR_SIZE_TYPE) {
        varSizeChunking();
    }

    if (ChunkerType == CHUNKER_TRACE_DRIVEN_TYPE_FSL) {
        traceDrivenChunkingFSL();
    }

    if (ChunkerType == CHUNKER_TRACE_DRIVEN_TYPE_UBC) {
        traceDrivenChunkingUBC();
    }

    return true;
}

void Chunker::fixSizeChunking()
{
#if SYSTEM_BREAK_DOWN == 1
    double insertTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    std::ifstream& fin = getChunkingFile();
    uint64_t chunkIDCounter = 0;
    memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
    uint64_t fileSize = 0;
    u_char hash[CHUNK_HASH_SIZE];
    /*start chunking*/
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartChunker, NULL);
#endif
    while (true) {
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunkerReadFile, NULL);
#endif
        memset((char*)waitingForChunkingBuffer, 0, sizeof(unsigned char) * ReadSize);
        fin.read((char*)waitingForChunkingBuffer, sizeof(char) * ReadSize);
        uint64_t totalReadSize = fin.gcount();
        fileSize += totalReadSize;
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunkerReadFile, NULL);
        diff = 1000000 * (timeendChunkerReadFile.tv_sec - timestartChunkerReadFile.tv_sec) + timeendChunkerReadFile.tv_usec - timestartChunkerReadFile.tv_usec;
        second = diff / 1000000.0;
        readFileTime += second;
#endif
        uint64_t chunkedSize = 0;
        if (totalReadSize == ReadSize) {
            while (chunkedSize < totalReadSize) {
                memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
                memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, avgChunkSize);
                Data_t tempChunk;
                tempChunk.chunk.ID = chunkIDCounter;
                tempChunk.chunk.logicDataSize = avgChunkSize;
                memcpy(tempChunk.chunk.logicData, chunkBuffer, avgChunkSize);
                memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                tempChunk.dataType = DATA_TYPE_CHUNK;
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
                insertMQToKeyClient(tempChunk);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendChunkerInsertMQ, NULL);
                diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
                second = diff / 1000000.0;
                insertTime += second;
#endif
                chunkIDCounter++;
                chunkedSize += avgChunkSize;
            }
        } else {
            uint64_t retSize = 0;
            while (chunkedSize < totalReadSize) {
                memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
                Data_t tempChunk;
                if (retSize > avgChunkSize) {

                    memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, avgChunkSize);

                    tempChunk.chunk.ID = chunkIDCounter;
                    tempChunk.chunk.logicDataSize = avgChunkSize;
                    memcpy(tempChunk.chunk.logicData, chunkBuffer, avgChunkSize);
                    memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                } else {

                    memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, retSize);

                    tempChunk.chunk.ID = chunkIDCounter;
                    tempChunk.chunk.logicDataSize = retSize;
                    memcpy(tempChunk.chunk.logicData, chunkBuffer, retSize);
                    memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                }
                retSize = totalReadSize - chunkedSize;
                tempChunk.dataType = DATA_TYPE_CHUNK;
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
                insertMQToKeyClient(tempChunk);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendChunkerInsertMQ, NULL);
                diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
                second = diff / 1000000.0;
                insertTime += second;
#endif
                chunkIDCounter++;
                chunkedSize += avgChunkSize;
            }
        }
        if (fin.eof()) {
            break;
        }
    }
    fileRecipe.recipe.fileRecipeHead.totalChunkNumber = chunkIDCounter;
    fileRecipe.recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCounter;
    fileRecipe.recipe.fileRecipeHead.fileSize = fileSize;
    fileRecipe.recipe.keyRecipeHead.fileSize = fileRecipe.recipe.fileRecipeHead.fileSize;
    fileRecipe.dataType = DATA_TYPE_RECIPE;
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
    if (!insertMQToKeyClient(fileRecipe)) {
        cerr << "Chunker : error insert recipe head to keyClient message queue" << endl;
        return;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendChunkerInsertMQ, NULL);
    diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
    second = diff / 1000000.0;
    insertTime += second;
#endif
    if (setJobDoneFlag() == false) {
        cerr << "Chunker : set chunking done flag error" << endl;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendChunker, NULL);
    diff = 1000000 * (timeendChunker.tv_sec - timestartChunker.tv_sec) + timeendChunker.tv_usec - timestartChunker.tv_usec;
    second = diff / 1000000.0;
    cerr << "Chunker : total read file time = " << setbase(10) << readFileTime << " s" << endl;
    cerr << "Chunker : total chunking time = " << setbase(10) << second - (insertTime + readFileTime) << " s" << endl;
#endif
    cerr << "Chunker : Fixed chunking over:\n\t  Total file size = " << fileRecipe.recipe.fileRecipeHead.fileSize << " Byte;\n\t  Total chunk number = " << fileRecipe.recipe.fileRecipeHead.totalChunkNumber << endl;
}

void Chunker::varSizeChunking()
{
#if SYSTEM_BREAK_DOWN == 1
    double insertTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    uint16_t winFp;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = 0;
    ifstream& fin = getChunkingFile();
    uint64_t fileSize = 0;
    u_char hash[CHUNK_HASH_SIZE];
/*start chunking*/
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartChunker, NULL);
#endif
    while (true) {
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunkerReadFile, NULL);
#endif
        memset((char*)waitingForChunkingBuffer, 0, sizeof(unsigned char) * ReadSize);
        fin.read((char*)waitingForChunkingBuffer, sizeof(unsigned char) * ReadSize);
        int len = fin.gcount();
        fileSize += len;
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunkerReadFile, NULL);
        diff = 1000000 * (timeendChunkerReadFile.tv_sec - timestartChunkerReadFile.tv_sec) + timeendChunkerReadFile.tv_usec - timestartChunkerReadFile.tv_usec;
        second = diff / 1000000.0;
        readFileTime += second;
#endif
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
                Data_t tempChunk;
                tempChunk.chunk.ID = chunkIDCnt;
                tempChunk.chunk.logicDataSize = chunkBufferCnt;
                memcpy(tempChunk.chunk.logicData, chunkBuffer, chunkBufferCnt);
                memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                tempChunk.dataType = DATA_TYPE_CHUNK;
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
                if (!insertMQToKeyClient(tempChunk)) {
                    cerr << "Chunker : error insert chunk to keyClient message queue for chunk ID = " << tempChunk.chunk.ID << endl;
                    return;
                }
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendChunkerInsertMQ, NULL);
                diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
                second = diff / 1000000.0;
                insertTime += second;
#endif
                chunkIDCnt++;
                chunkBufferCnt = winFp = 0;
            }

            /*chunk's size exceed maxChunkSize*/
            if (chunkBufferCnt >= maxChunkSize) {
                Data_t tempChunk;
                tempChunk.chunk.ID = chunkIDCnt;
                tempChunk.chunk.logicDataSize = chunkBufferCnt;
                memcpy(tempChunk.chunk.logicData, chunkBuffer, chunkBufferCnt);
                memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                tempChunk.dataType = DATA_TYPE_CHUNK;
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
                if (!insertMQToKeyClient(tempChunk)) {
                    cerr << "Chunker : error insert chunk to keyClient message queue for chunk ID = " << tempChunk.chunk.ID << endl;
                    return;
                }
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendChunkerInsertMQ, NULL);
                diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
                second = diff / 1000000.0;
                insertTime += second;
#endif
                chunkIDCnt++;
                chunkBufferCnt = winFp = 0;
            }
        }
        if (fin.eof()) {
            break;
        }
    }

    /*add final chunk*/
    if (chunkBufferCnt != 0) {
        Data_t tempChunk;
        tempChunk.chunk.ID = chunkIDCnt;
        tempChunk.chunk.logicDataSize = chunkBufferCnt;
        memcpy(tempChunk.chunk.logicData, chunkBuffer, chunkBufferCnt);
        memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
        tempChunk.dataType = DATA_TYPE_CHUNK;
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
        if (!insertMQToKeyClient(tempChunk)) {
            cerr << "Chunker : error insert chunk to keyClient message queue for chunk ID = " << tempChunk.chunk.ID << endl;
            return;
        }
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunkerInsertMQ, NULL);
        diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
        second = diff / 1000000.0;
        insertTime += second;
#endif
        chunkIDCnt++;
        chunkBufferCnt = winFp = 0;
    }
    fileRecipe.recipe.fileRecipeHead.totalChunkNumber = chunkIDCnt;
    fileRecipe.recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCnt;
    fileRecipe.recipe.fileRecipeHead.fileSize = fileSize;
    fileRecipe.recipe.keyRecipeHead.fileSize = fileRecipe.recipe.fileRecipeHead.fileSize;
    fileRecipe.dataType = DATA_TYPE_RECIPE;
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartChunkerInsertMQ, NULL);
#endif
    if (!insertMQToKeyClient(fileRecipe)) {
        cerr << "Chunker : error insert recipe head to keyClient message queue" << endl;
        return;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendChunkerInsertMQ, NULL);
    diff = 1000000 * (timeendChunkerInsertMQ.tv_sec - timestartChunkerInsertMQ.tv_sec) + timeendChunkerInsertMQ.tv_usec - timestartChunkerInsertMQ.tv_usec;
    second = diff / 1000000.0;
    insertTime += second;
#endif
    if (setJobDoneFlag() == false) {
        cerr << "Chunker: set chunking done flag error" << endl;
        return;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendChunker, NULL);
    diff = 1000000 * (timeendChunker.tv_sec - timestartChunker.tv_sec) + timeendChunker.tv_usec - timestartChunker.tv_usec;
    second = diff / 1000000.0;
    cerr << "Chunker : total read file time = " << setbase(10) << readFileTime << " s" << endl;
    cerr << "Chunker : total chunking time = " << setbase(10) << second - (insertTime + readFileTime) << " s" << endl;
#endif
    cerr << "Chunker : variable size chunking over:\n\t  Total file size = " << fileRecipe.recipe.fileRecipeHead.fileSize << " Byte;\n\t  Total chunk number = " << fileRecipe.recipe.fileRecipeHead.totalChunkNumber << endl;
    return;
}

void Chunker::traceDrivenChunkingFSL()
{
    char* readFlag;
#if SYSTEM_BREAK_DOWN == 1
    double chunkTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    std::ifstream& fin = getChunkingFile();
    uint64_t chunkIDCounter = 0;
    uint64_t fileSize = 0;
    u_char hash[CHUNK_HASH_SIZE];
    char readLineBuffer[256];
    string readLineStr;
    /*start chunking*/
    getline(fin, readLineStr);
    while (true) {
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunkerReadFile, NULL);
#endif
        getline(fin, readLineStr);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunkerReadFile, NULL);
        diff = 1000000 * (timeendChunkerReadFile.tv_sec - timestartChunkerReadFile.tv_sec) + timeendChunkerReadFile.tv_usec - timestartChunkerReadFile.tv_usec;
        second = diff / 1000000.0;
        readFileTime += second;
#endif
        if (fin.eof()) {
            break;
        }
        memset(readLineBuffer, 0, 256);
        memcpy(readLineBuffer, (char*)readLineStr.c_str(), readLineStr.length());
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunker, NULL);
#endif
        u_char chunkFp[7];
        memset(chunkFp, 0, 7);
        char* item;
        item = strtok(readLineBuffer, ":\t\n ");
        for (int index = 0; item != NULL && index < 6; index++) {
            chunkFp[index] = strtol(item, NULL, 16);
            item = strtok(NULL, ":\t\n");
        }
        chunkFp[6] = '\0';
        auto size = atoi(item);
        int copySize = 0;
        memset(chunkBuffer, 0, sizeof(char) * maxChunkSize + 6);
        if (size > maxChunkSize) {
            continue;
        }
        while (copySize < size) {
            memcpy(chunkBuffer + copySize, chunkFp, 6);
            copySize += 6;
        }
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunker, NULL);
        diff = 1000000 * (timeendChunker.tv_sec - timestartChunker.tv_sec) + timeendChunker.tv_usec - timestartChunker.tv_usec;
        second = diff / 1000000.0;
        chunkTime += second;
#endif
        Data_t tempChunk;
        tempChunk.chunk.ID = chunkIDCounter;
        tempChunk.chunk.logicDataSize = size;
        memcpy(tempChunk.chunk.logicData, chunkBuffer, size);
        memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
        tempChunk.dataType = DATA_TYPE_CHUNK;

        insertMQToKeyClient(tempChunk);
        chunkIDCounter++;
        fileSize += size;
    }
    fileRecipe.recipe.fileRecipeHead.totalChunkNumber = chunkIDCounter;
    fileRecipe.recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCounter;
    fileRecipe.recipe.fileRecipeHead.fileSize = fileSize;
    fileRecipe.recipe.keyRecipeHead.fileSize = fileRecipe.recipe.fileRecipeHead.fileSize;
    fileRecipe.dataType = DATA_TYPE_RECIPE;
    insertMQToKeyClient(fileRecipe);
    if (setJobDoneFlag() == false) {
        cerr << "Chunker : set chunking done flag error" << endl;
    }
#if SYSTEM_BREAK_DOWN == 1
    cerr << "Chunker : total read file time = " << setbase(10) << readFileTime << " s" << endl;
    cerr << "Chunker : total chunking time = " << chunkTime << " s" << endl;
#endif
    cerr << "Chunker : trace gen over:\n\t  Total file size = " << fileRecipe.recipe.fileRecipeHead.fileSize << " Byte;\n\t  Total chunk number = " << fileRecipe.recipe.fileRecipeHead.totalChunkNumber << endl;
}

void Chunker::traceDrivenChunkingUBC()
{
    char* readFlag;
#if SYSTEM_BREAK_DOWN == 1
    double chunkTime = 0;
    double readFileTime = 0;
    long diff;
    double second;
#endif
    std::ifstream& fin = getChunkingFile();
    uint64_t chunkIDCounter = 0;
    uint64_t fileSize = 0;
    u_char hash[CHUNK_HASH_SIZE];
    char readLineBuffer[256];
    string readLineStr;
    /*start chunking*/
    getline(fin, readLineStr);
    while (true) {
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunkerReadFile, NULL);
#endif
        getline(fin, readLineStr);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunkerReadFile, NULL);
        diff = 1000000 * (timeendChunkerReadFile.tv_sec - timestartChunkerReadFile.tv_sec) + timeendChunkerReadFile.tv_usec - timestartChunkerReadFile.tv_usec;
        second = diff / 1000000.0;
        readFileTime += second;
#endif
        if (fin.eof()) {
            break;
        }
        memset(readLineBuffer, 0, 256);
        memcpy(readLineBuffer, (char*)readLineStr.c_str(), readLineStr.length());

#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartChunker, NULL);
#endif
        u_char chunkFp[6];
        memset(chunkFp, 0, 6);
        char* item;
        item = strtok(readLineBuffer, ":\t\n ");
        for (int index = 0; item != NULL && index < 5; index++) {
            chunkFp[index] = strtol(item, NULL, 16);
            item = strtok(NULL, ":\t\n");
        }
        chunkFp[5] = '\0';
        auto size = atoi(item);
        int copySize = 0;
        memset(chunkBuffer, 0, sizeof(char) * maxChunkSize + 5);
        if (size > maxChunkSize) {
            continue;
        }
        while (copySize < size) {
            memcpy(chunkBuffer + copySize, chunkFp, 5);
            copySize += 5;
        }
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendChunker, NULL);
        diff = 1000000 * (timeendChunker.tv_sec - timestartChunker.tv_sec) + timeendChunker.tv_usec - timestartChunker.tv_usec;
        second = diff / 1000000.0;
        chunkTime += second;
#endif
        Data_t tempChunk;
        tempChunk.chunk.ID = chunkIDCounter;
        tempChunk.chunk.logicDataSize = size;
        memcpy(tempChunk.chunk.logicData, chunkBuffer, size);
        memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
        tempChunk.dataType = DATA_TYPE_CHUNK;

        insertMQToKeyClient(tempChunk);
        chunkIDCounter++;
        fileSize += size;
    }
    fileRecipe.recipe.fileRecipeHead.totalChunkNumber = chunkIDCounter;
    fileRecipe.recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCounter;
    fileRecipe.recipe.fileRecipeHead.fileSize = fileSize;
    fileRecipe.recipe.keyRecipeHead.fileSize = fileRecipe.recipe.fileRecipeHead.fileSize;
    fileRecipe.dataType = DATA_TYPE_RECIPE;
    insertMQToKeyClient(fileRecipe);
    if (setJobDoneFlag() == false) {
        cerr << "Chunker : set chunking done flag error" << endl;
    }
#if SYSTEM_BREAK_DOWN == 1
    cerr << "Chunker : total read file time = " << setbase(10) << readFileTime << " s" << endl;
    cerr << "Chunker : total chunking time is" << chunkTime << " s" << endl;
#endif
    cerr << "Chunker : trace gen over:\n\t  Total file size = " << fileRecipe.recipe.fileRecipeHead.fileSize << " Byte;\n\t  Total chunk number = " << fileRecipe.recipe.fileRecipeHead.totalChunkNumber << endl;
}

bool Chunker::insertMQToKeyClient(Data_t& newData)
{
    return keyClientObj->insertMQFromChunker(newData);
}

bool Chunker::setJobDoneFlag()
{
    return keyClientObj->editJobDoneFlag();
}
