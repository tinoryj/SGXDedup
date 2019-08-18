#include "retriever.hpp"
#include "../pow/include/hexutil.h"

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
    RetrieverData_t newData;
    while (totalRecvNumber_ < totalChunkNumber_) {
        if (extractMQFromRecvDecode(newData)) {
            retrieveFile_.write(newData.logicData, newData.logicDataSize);
            totalRecvNumber_++;
        }
    }
    cerr << "Retriever : job done, thread exit now" << endl;
    return;
}

bool Retriever::extractMQFromRecvDecode(RetrieverData_t& newData)
{
    return recvDecodeObj_->extractMQToRetriever(newData);
}