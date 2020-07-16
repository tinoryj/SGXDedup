#include "recvDecode.hpp"

extern Configure config;

struct timeval timestartRecvDecode;
struct timeval timeendRecvDecode;

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
    dataSecurityChannel_ = new ssl(config.getStorageServerIP(), config.getStorageServerPort(), CLIENTSIDE);
    powSecurityChannel_ = new ssl(config.getStorageServerIP(), config.getPOWServerPort(), CLIENTSIDE);
    sslConnectionData_ = dataSecurityChannel_->sslConnect().second;
    sslConnectionPow_ = powSecurityChannel_->sslConnect().second;
#if SYSTEM_BREAK_DOWN == 1
    long diff;
    double second;
    gettimeofday(&timestartRecvDecode, NULL);
#endif
    cryptoObj_->generateHash((u_char*)&fileName[0], fileName.length(), fileNameHash_);
    recvFileHead(fileRecipe_, fileNameHash_);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendRecvDecode, NULL);
    diff = 1000000 * (timeendRecvDecode.tv_sec - timestartRecvDecode.tv_sec) + timeendRecvDecode.tv_usec - timestartRecvDecode.tv_usec;
    second = diff / 1000000.0;
    cerr << "RecvDecode : recv file head infomation time = " << second << " s" << endl;
#endif
    cerr << "RecvDecode : recv file recipe head, file size = " << fileRecipe_.fileRecipeHead.fileSize << ", total chunk number = " << fileRecipe_.fileRecipeHead.totalChunkNumber << endl;
}

RecvDecode::~RecvDecode()
{
    NetworkHeadStruct_t request;
    request.messageType = CLIENT_EXIT;
    request.dataSize = 0;
    request.clientID = clientID_;
    int sendSize = sizeof(NetworkHeadStruct_t);
    char requestBuffer[sendSize];
    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    dataSecurityChannel_->send(sslConnectionData_, requestBuffer, sendSize);
    powSecurityChannel_->send(sslConnectionPow_, requestBuffer, sendSize);
    delete dataSecurityChannel_;
    delete powSecurityChannel_;
    if (cryptoObj_ != nullptr) {
        delete cryptoObj_;
    }
#if QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_SPSC_QUEUE || QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_QUEUE
    inputMQ_->~messageQueue();
    delete inputMQ_;
#endif
}

bool RecvDecode::recvFileHead(Recipe_t& fileRecipe, u_char* fileNameHash)
{
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_FILEHEAD;
    request.dataSize = FILE_NAME_HASH_SIZE;
    request.clientID = clientID_;

    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE;
    char requestBuffer[sendSize];
    char respondBuffer[1024 * 1024];
    int recvSize;

    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash, FILE_NAME_HASH_SIZE);

    while (true) {
        if (!dataSecurityChannel_->send(sslConnectionData_, requestBuffer, sendSize)) {
            cerr << "RecvDecode : storage server closed" << endl;
            return false;
        }
        if (!dataSecurityChannel_->recv(sslConnectionData_, respondBuffer, recvSize)) {
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
#if SYSTEM_BREAK_DOWN == 1
    long diff;
    double second;
    double decryptChunkKeyTime = 0;
    double decryptChunkContentTime = 0;
#endif
    NetworkHeadStruct_t request, respond;
    request.messageType = CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE;
    request.dataSize = FILE_NAME_HASH_SIZE + 2 * sizeof(uint32_t);
    request.clientID = clientID_;
    int sendSize = sizeof(NetworkHeadStruct_t) + FILE_NAME_HASH_SIZE;
    char requestBuffer[sendSize];
    char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;

    memcpy(requestBuffer, &request, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), fileNameHash_, FILE_NAME_HASH_SIZE);

    if (!dataSecurityChannel_->send(sslConnectionData_, requestBuffer, sendSize)) {
        cerr << "RecvDecode : storage server closed" << endl;
        return;
    }
    while (totalRecvChunks < fileRecipe_.fileRecipeHead.totalChunkNumber) {

        if (!dataSecurityChannel_->recv(sslConnectionData_, respondBuffer, recvSize)) {
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
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartRecvDecode, NULL);
#endif
                u_char plaintKey[CHUNK_ENCRYPT_KEY_SIZE];
                cryptoObj_->decryptWithKey((u_char*)respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize + chunkSize, CHUNK_ENCRYPT_KEY_SIZE, cryptoObj_->chunkKeyEncryptionKey_, plaintKey);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendRecvDecode, NULL);
                diff = 1000000 * (timeendRecvDecode.tv_sec - timestartRecvDecode.tv_sec) + timeendRecvDecode.tv_usec - timestartRecvDecode.tv_usec;
                second = diff / 1000000.0;
                decryptChunkKeyTime += second;
#endif
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartRecvDecode, NULL);
#endif
                cryptoObj_->decryptWithKey((u_char*)respondBuffer + sizeof(NetworkHeadStruct_t) + totalRecvSize, chunkSize, plaintKey, chunkPlaintData);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendRecvDecode, NULL);
                diff = 1000000 * (timeendRecvDecode.tv_sec - timestartRecvDecode.tv_sec) + timeendRecvDecode.tv_usec - timestartRecvDecode.tv_usec;
                second = diff / 1000000.0;
                decryptChunkContentTime += second;
#endif
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
#if SYSTEM_BREAK_DOWN == 1
    cerr << "RecvDecode : chunk key decrypt time = " << decryptChunkKeyTime << " s" << endl;
    cerr << "RecvDecode : chunk content decrypt time = " << decryptChunkContentTime << " s" << endl;
#endif
    return;
}