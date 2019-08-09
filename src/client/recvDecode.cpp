#include "recvDecode.hpp"

extern Configure config;

RecvDecode::RecvDecode(string fileName)
{
    outPutMQ_ = new messageQueue<RetrieverData_t>(config.get_RetrieverData_t_MQSize());
    cryptoObj_ = new CryptoPrimitive();
    socket_.init(CLIENT_TCP, config.getStorageServerIP(), config.getStorageServerPort());
    cryptoObj_->generateHash((u_char*)&fileName[0], fileName.length(), fileNameHash_);
    recvFileHead(fileRecipe_, fileNameHash_);
    cerr << "RecvDecode : recv file recipe head, file size = " << fileRecipe_.fileRecipeHead.fileSize << ", total chunk number = " << fileRecipe_.fileRecipeHead.totalChunkNumber << endl;
}

RecvDecode::~RecvDecode()
{
    socket_.finish();
    if (cryptoObj_ != nullptr) {
        delete cryptoObj_;
    }
    delete outPutMQ_;
}

bool RecvDecode::recvFileHead(Recipe_t& fileRecipe, u_char* fileNameHash)
{
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_FILEHEAD;
    request.dataSize = FILE_NAME_HASH_SIZE;
    request.clientID = config.getClientID();

    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE;
    u_char requestBuffer[sendSize];
    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;

    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash, FILE_NAME_HASH_SIZE);

    while (true) {
        if (!socket_.Send(requestBuffer, sendSize)) {
            cerr << "RecvDecode : storage server closed" << endl;
            return false;
        }
        if (!socket_.Recv(respondBuffer, recvSize)) {
            cerr << "RecvDecode : storage server closed" << endl;
            return false;
        }
        memcpy(&respond, respondBuffer, sizeof(NetworkHeadStruct_t));
        if (respond.messageType == ERROR_RESEND) {
            std::cerr << "RecvDecode : Server send resend flag!" << endl;
            continue;
        }
        if (respond.messageType == ERROR_CLOSE) {
            std::cerr << "RecvDecode : Server reject download request!" << endl;
            exit(1);
        }
        if (respond.messageType == ERROR_FILE_NOT_EXIST) {
            std::cerr << "RecvDecode : Server reject download request, file not exist in server!" << endl;
            exit(1);
        }
        if (respond.messageType == ERROR_CHUNK_NOT_EXIST) {
            std::cerr << "RecvDecode : Server reject download request, chunk not exist in server!" << endl;
            exit(1);
        }
        if (respond.messageType == SUCCESS) {
            if (respond.dataSize != sizeof(Recipe_t)) {
                std::cerr << "RecvDecode : Server send file recipe head faild!" << endl;
                exit(1);
            } else {
                memcpy(&fileRecipe, respondBuffer + sizeof(NetworkHeadStruct_t), sizeof(Recipe_t));
                break;
            }
        }
    }
    return true;
}

bool RecvDecode::recvChunks(ChunkList_t& recvChunk, int& chunkNumber, uint32_t& startID, uint32_t& endID)
{
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE;
    request.dataSize = FILE_NAME_HASH_SIZE + 2 * sizeof(uint32_t);
    request.clientID = config.getClientID();
    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE + 2 * sizeof(uint32_t);
    u_char requestBuffer[sendSize];
    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;
    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash_, FILE_NAME_HASH_SIZE);
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE, &startID, sizeof(uint32_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE + sizeof(uint32_t), &endID, sizeof(uint32_t));
    while (true) {
        if (!socket_.Send(requestBuffer, sendSize)) {
            cerr << "RecvDecode : storage server closed" << endl;
            return false;
        }
        if (!socket_.Recv(respondBuffer, recvSize)) {
            cerr << "RecvDecode : storage server closed" << endl;
            return false;
        }
        memcpy(&respond, respondBuffer, sizeof(NetworkHeadStruct_t));
        if (respond.messageType == ERROR_RESEND)
            continue;
        if (respond.messageType == ERROR_CLOSE) {
            std::cerr << "RecvDecode : Server reject download request!" << endl;
            exit(1);
        }
        if (respond.messageType == SUCCESS) {
            chunkNumber = endID - startID;
            int totalRecvSize = 0;
            for (int i = 0; i < chunkNumber; i++) {
                Chunk_t tempChunk;
                memcpy(&tempChunk.ID, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, sizeof(uint32_t));
                totalRecvSize += sizeof(uint32_t);
                memcpy(&tempChunk.logicDataSize, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, sizeof(int));
                totalRecvSize += sizeof(int);
                memcpy(&tempChunk.logicData, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, tempChunk.logicDataSize);
                totalRecvSize += tempChunk.logicDataSize;
                memcpy(&tempChunk.encryptKey, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, CHUNK_ENCRYPT_KEY_SIZE);
                totalRecvSize += CHUNK_ENCRYPT_KEY_SIZE;
                recvChunk.push_back(tempChunk);
                //cerr << "RecvDecode : recv chunk id = " << tempChunk.ID << endl;
            }
            break;
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
    return outPutMQ_->push(newData);
}
bool RecvDecode::extractMQToRetriever(RetrieverData_t& newData)
{
    return outPutMQ_->pop(newData);
}

void RecvDecode::run()
{
    while (totalRecvChunks < fileRecipe_.fileRecipeHead.totalChunkNumber) {
        ChunkList_t newChunkList;
        int chunkNumber = 0;
        uint32_t startID = totalRecvChunks;
        uint32_t endID;
        if ((fileRecipe_.fileRecipeHead.totalChunkNumber - totalRecvChunks) > config.getSendChunkBatchSize()) {
            endID = startID + config.getSendChunkBatchSize();
        } else {
            endID = startID + fileRecipe_.fileRecipeHead.totalChunkNumber - totalRecvChunks;
        }
        cerr << "RecvDecode : current request chunks from " << startID << " to " << endID << endl;
        if (recvChunks(newChunkList, chunkNumber, startID, endID)) {
            for (int i = 0; i < chunkNumber; i++) {
                cryptoObj_->decryptChunk(newChunkList[i]);
                RetrieverData_t newData;
                newData.ID = newChunkList[i].ID;
                newData.logicDataSize = newChunkList[i].logicDataSize;
                memcpy(newData.logicData, newChunkList[i].logicData, newChunkList[i].logicDataSize);
                if (!insertMQToRetriever(newData)) {
                    cerr << "RecvDecode : Error insert chunk data into retriever" << endl;
                }
            }
            newChunkList.clear();
            // multiThreadDownloadMutex.lock();
            totalRecvChunks += chunkNumber;
            // multiThreadDownloadMutex.unlock();
            // cerr << "RecvDecode : recv " << chunkNumber << " chunks done" << endl;
        } else {
            break;
        }
    }
    cerr << "RecvDecode : download job done, exit now" << endl;
    return;
}