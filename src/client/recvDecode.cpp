#include "recvDecode.hpp"

extern Configure config;

RecvDecode::RecvDecode(string fileName)
{
    cryptoObj_ = new CryptoPrimitive();
    configObj_ = new Configure("config.json");
    socket_.init(CLIENT_TCP, configObj_->getStorageServerIP(), configObj_->getStorageServerPort());
    cryptoObj_->generateHash((u_char*)&fileName[0], fileName.length(), fileNameHash_);
    recvFileHead(fileRecipe_, fileNameHash_);
}

RecvDecode::~RecvDecode()
{
    socket_.finish();
    if (cryptoObj_ != nullptr) {
        delete cryptoObj_;
    }
    if (configObj_ != nullptr) {
        delete configObj_;
    }
}

bool RecvDecode::recvFileHead(Recipe_t& fileRecipe, u_char* fileNameHash)
{
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_RECIPE;
    request.dataSize = FILE_NAME_HASH_SIZE;
    request.clientID = configObj_->getClientID();

    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE;
    u_char requestBuffer[sendSize];
    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;

    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash, FILE_NAME_HASH_SIZE);

    while (true) {
        if (!socket_.Send(requestBuffer, sendSize)) {
            cerr << "storage server closed" << endl;
            return false;
        }
        if (!socket_.Recv(respondBuffer, recvSize)) {
            cerr << "storage server closed" << endl;
            return false;
        }
        memcpy(&respond, respondBuffer, sizeof(NetworkHeadStruct_t));
        if (respond.messageType == ERROR_RESEND)
            continue;
        if (respond.messageType == ERROR_CLOSE) {
            std::cerr << "Server reject download request!" << endl;
            exit(1);
        }
        if (respond.messageType == SUCCESS) {
            if (respond.dataSize != sizeof(Recipe_t)) {
                std::cerr << "Server send file recipe head faild!" << endl;
                exit(1);
            } else {
                memcpy(&fileRecipe, respondBuffer + sizeof(NetworkHeadStruct_t), sizeof(Recipe_t));
                break;
            }
        }
    }
    return true;
}

bool RecvDecode::recvChunks(ChunkList_t& recvChunk, int& chunkNumber)
{
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_CHUNK;
    request.dataSize = FILE_NAME_HASH_SIZE;
    request.clientID = configObj_->getClientID();
    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE + 2 * sizeof(uint32_t);
    u_char requestBuffer[sendSize];
    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;
    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash_, FILE_NAME_HASH_SIZE);

    while (true) {
        if (!socket_.Send(requestBuffer, sendSize)) {
            cerr << "storage server closed" << endl;
            return false;
        }
        if (!socket_.Recv(respondBuffer, recvSize)) {
            cerr << "storage server closed" << endl;
            return false;
        }
        memcpy(&respond, respondBuffer, sizeof(NetworkHeadStruct_t));
        if (respond.messageType == ERROR_RESEND)
            continue;
        if (respond.messageType == ERROR_CLOSE) {
            std::cerr << "Server reject download request!" << endl;
            exit(1);
        }
        if (respond.messageType == SUCCESS) {
            if ((respond.dataSize / sizeof(Chunk_t)) * sizeof(Chunk_t) != respond.dataSize) {
                std::cerr << "Server send chunks faild!" << endl;
                exit(1);
            } else {
                chunkNumber = respond.dataSize / sizeof(Chunk_t);
                for (int i = 0; i < chunkNumber; i++) {
                    Chunk_t tempChunk;
                    memcpy(&tempChunk, respondBuffer + sizeof(NetworkHeadStruct_t) + i * sizeof(Chunk_t), sizeof(Chunk_t));
                    recvChunk.push_back(tempChunk);
                }
                break;
            }
        }
    }
    return true;
}

Recipe_t RecvDecode::getFileRecipeHead()
{
    return fileRecipe_;
}

bool RecvDecode::insertMQToRetriever(RetrieverData_t& newData)
{
    return outPutMQ.push(newData);
}
bool RecvDecode::extractMQToRetriever(RetrieverData_t& newData)
{
    return outPutMQ.pop(newData);
}

void RecvDecode::run()
{
    while (totalRecvChunks < fileRecipe_.fileRecipeHead.totalChunkNumber) {
        ChunkList_t newChunkList;
        int chunkNumber = 0;
        recvChunks(newChunkList, chunkNumber);
        for (int i = 0; i < chunkNumber; i++) {
            cryptoObj_->decryptChunk(newChunkList[i]);
            RetrieverData_t newData;
            newData.ID = newChunkList[i].ID;
            newData.logicDataSize = newChunkList[i].logicDataSize;
            memcpy(newData.logicData, newChunkList[i].logicData, newChunkList[i].logicDataSize);
            if (!insertMQToRetriever(newData)) {
                cerr << "Error insert chunk data into retriever" << endl;
            }
        }
        newChunkList.clear();
        multiThreadDownloadMutex.lock();
        totalRecvChunks += chunkNumber;
        multiThreadDownloadMutex.unlock();
    }
    pthread_exit(NULL);
}