#include "recvDecode.hpp"

extern Configure config;

void PRINT_BYTE_ARRAY_RECV(
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

RecvDecode::RecvDecode(string fileName)
{
    clientID_ = config.getClientID();
    outPutMQ_ = new messageQueue<RetrieverData_t>;
    cryptoObj_ = new CryptoPrimitive();
    socket_.init(CLIENT_TCP, config.getStorageServerIP(), config.getStorageServerPort());
    socketPow_.init(CLIENT_TCP, config.getStorageServerIP(), config.getPOWServerPort());
    cryptoObj_->generateHash((u_char*)&fileName[0], fileName.length(), fileNameHash_);
    recvFileHead(fileRecipe_, fileNameHash_);
    cerr << "RecvDecode : recv file recipe head, file size = " << fileRecipe_.fileRecipeHead.fileSize << ", total chunk number = " << fileRecipe_.fileRecipeHead.totalChunkNumber << endl;
}

RecvDecode::~RecvDecode()
{
    socket_.finish();
    socketPow_.finish();
    if (cryptoObj_ != nullptr) {
        delete cryptoObj_;
    }
    outPutMQ_->~messageQueue();
    delete outPutMQ_;
}

bool RecvDecode::recvFileHead(Recipe_t& fileRecipe, u_char* fileNameHash)
{
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_FILEHEAD;
    request.dataSize = FILE_NAME_HASH_SIZE;
    request.clientID = clientID_;

    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE;
    u_char requestBuffer[sendSize];
    u_char respondBuffer[1024 * 1024];
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
            cerr << "RecvDecode : Server send resend flag!" << endl;
            continue;
        }
        if (respond.messageType == ERROR_CLOSE) {
            cerr << "RecvDecode : Server reject download request!" << endl;
            exit(1);
        }
        if (respond.messageType == ERROR_FILE_NOT_EXIST) {
            cerr << "RecvDecode : Server reject download request, file not exist in server!" << endl;
            exit(1);
        }
        if (respond.messageType == ERROR_CHUNK_NOT_EXIST) {
            cerr << "RecvDecode : Server reject download request, chunk not exist in server!" << endl;
            exit(1);
        }
        if (respond.messageType == SUCCESS) {
            if (respond.dataSize != sizeof(Recipe_t)) {
                cerr << "RecvDecode : Server send file recipe head faild!" << endl;
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
    request.clientID = clientID_;
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
            cerr << "RecvDecode : Server reject download request!" << endl;
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
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE;
    request.dataSize = FILE_NAME_HASH_SIZE + 2 * sizeof(uint32_t);
    request.clientID = clientID_;
    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE;
    u_char requestBuffer[sendSize];
    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;

    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash_, FILE_NAME_HASH_SIZE);

    if (!socket_.Send(requestBuffer, sendSize)) {
        cerr << "RecvDecode : storage server closed" << endl;
        return;
    }
    while (totalRecvChunks < fileRecipe_.fileRecipeHead.totalChunkNumber) {

        if (!socket_.Recv(respondBuffer, recvSize)) {
            cerr << "RecvDecode : storage server closed" << endl;
            return;
        }
        memcpy(&respond, respondBuffer, sizeof(NetworkHeadStruct_t));
        if (respond.messageType == ERROR_RESEND)
            continue;
        if (respond.messageType == ERROR_CLOSE) {
            cerr << "RecvDecode : Server reject download request!" << endl;
            return;
        }
        if (respond.messageType == SUCCESS) {
            int totalRecvSize = sizeof(int);
            int chunkSize;
            uint32_t chunkID;
            int chunkNumber;
            u_char chunkPlaintData[MAX_CHUNK_SIZE];
            memcpy(&chunkNumber, respondBuffer + sizeof(NetworkHeadStruct_t), sizeof(int));
            for (int i = 0; i < chunkNumber; i++) {
                memcpy(&chunkID, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, sizeof(uint32_t));
                totalRecvSize += sizeof(uint32_t);
                memcpy(&chunkSize, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, sizeof(int));
                totalRecvSize += sizeof(int);
                cryptoObj_->decryptChunk(respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, chunkSize, respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize + chunkSize, chunkPlaintData);

                RetrieverData_t newData;
                newData.ID = chunkID;
                newData.logicDataSize = chunkSize;
                memcpy(newData.logicData, chunkPlaintData, chunkSize);
                if (!insertMQToRetriever(newData)) {
                    cerr << "RecvDecode : Error insert chunk data into retriever" << endl;
                }
                totalRecvSize = totalRecvSize + chunkSize + CHUNK_ENCRYPT_KEY_SIZE;
            }
            totalRecvChunks += chunkNumber;
        }
    }
    cerr << "RecvDecode : download job done, exit now" << endl;
    return;
}