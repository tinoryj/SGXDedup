#include "retriever.hpp"
#include "../pow/include/hexutil.h"

struct timeval timestartRetriever;
struct timeval timeendRetriever;

Retriever::Retriever(string fileName, RecvDecode*& recvDecodeObjTemp)
{
    recvDecodeObj_ = recvDecodeObjTemp;
    string newFileName = fileName.append(".d");
    retrieveFile_.open(newFileName, ofstream::out | ofstream::binary);
    Recipe_t tempRecipe = recvDecodeObj_->getFileRecipeHead();
    totalChunkNumber_ = tempRecipe.fileRecipeHead.totalChunkNumber;
}

Retriever::~Retriever()
{
    retrieveFile_.close();
}

void Retriever::recvThread()
{
#if SYSTEM_BREAK_DOWN == 1
    long diff;
    double second;
    double writeFileTime = 0;
#endif
    RetrieverData_t newData;
    while (totalRecvNumber_ < totalChunkNumber_) {
        if (extractMQFromRecvDecode(newData)) {
#if SYSTEM_BREAK_DOWN == 1
            long diff;
            double second;
            gettimeofday(&timestartRetriever, NULL);
#endif
            retrieveFile_.write(newData.logicData, newData.logicDataSize);
            totalRecvNumber_++;
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendRetriever, NULL);
            diff = 1000000 * (timeendRetriever.tv_sec - timestartRetriever.tv_sec) + timeendRetriever.tv_usec - timestartRetriever.tv_usec;
            second = diff / 1000000.0;
            writeFileTime += second;
#endif
        }
    }
#if SYSTEM_BREAK_DOWN == 1
    cerr << "Retriever : write file time = " << writeFileTime << " s" << endl;
#endif
    cerr << "Retriever : job done, thread exit now" << endl;
    return;
}

bool Retriever::extractMQFromRecvDecode(RetrieverData_t& newData)
{
    return recvDecodeObj_->extractMQToRetriever(newData);
}