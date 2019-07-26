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

bool Retriever::Retrieve()
{
    while (currentID_ < totalChunkNumber_) {
        multiThreadWriteMutex.lock();
        auto current = chunkTempList_.find(currentID_);
        if (current == chunkTempList_.end()) {
            continue;
        } else {
            retrieveFile_.write(current->second.c_str(), current->second.length());
            chunkTempList_.erase(current);
            currentID_++;
        }
        multiThreadWriteMutex.unlock();
    }
    return true;
}

void Retriever::run()
{
    while (totalRecvNumber_ < totalChunkNumber_) {
        RetrieverData_t newData;
        if (!extractMQFromRecvDecode(newData)) {
            cerr << "Error extract mq in retriever" << endl;
        }
        string data;
        data.resize(newData.logicDataSize);
        memcpy(&data[0], newData.logicData, newData.logicDataSize);
        multiThreadWriteMutex.lock();
        chunkTempList_.insert(make_pair(newData.ID, data));
        multiThreadWriteMutex.unlock();
    }
}

bool Retriever::extractMQFromRecvDecode(RetrieverData_t& newData)
{
    return recvDecodeObj_->extractMQToRetriever(newData);
}