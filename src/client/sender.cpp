#include "sender.hpp"
#include <sys/time.h>

extern Configure config;

struct timeval timestartSender;
struct timeval timeendSender;
struct timeval timestartSenderRecipe;
struct timeval timeendSenderRecipe;

void PRINT_BYTE_ARRAY_SENDER(
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

Sender::Sender()
{
    inputMQ_ = new messageQueue<Data_t>(config.get_Data_t_MQSize());
    socket_.init(CLIENT_TCP, config.getStorageServerIP(), config.getStorageServerPort());
    cryptoObj_ = new CryptoPrimitive();
    clientID_ = config.getClientID();
}

Sender::~Sender()
{
    socket_.finish();
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
    inputMQ_->~messageQueue();
    delete inputMQ_;
}

bool Sender::sendRecipe(Recipe_t request, RecipeList_t recipeList, int& status)
{
    // cerr << "Sender : send file recipe head, file size = " << request.fileRecipeHead.fileSize << " file chunk number = " << request.fileRecipeHead.totalChunkNumber << endl;
    // PRINT_BYTE_ARRAY_SENDER(stderr, request.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
    int totalRecipeNumber = recipeList.size();
    cerr << "Sender : Start sending file recipes, total recipe entry = " << totalRecipeNumber << endl;
    int sendRecipeNumber = 0;
    int sendRecipeBatchNumber = config.getSendRecipeBatchSize();
    int currentSendRecipeNumber = 0;
    while ((totalRecipeNumber - sendRecipeNumber) != 0) {

        if (totalRecipeNumber - sendRecipeNumber < sendRecipeBatchNumber) {
            currentSendRecipeNumber = totalRecipeNumber - sendRecipeNumber;
        } else {
            currentSendRecipeNumber = sendRecipeBatchNumber;
        }
        NetworkHeadStruct_t requestBody, respondBody;

        requestBody.clientID = clientID_;
        requestBody.messageType = CLIENT_UPLOAD_RECIPE;
        respondBody.clientID = 0;
        respondBody.messageType = 0;
        respondBody.dataSize = 0;
        int sendSize = sizeof(NetworkHeadStruct_t) + sizeof(Recipe_t) + currentSendRecipeNumber * sizeof(RecipeEntry_t);
        requestBody.dataSize = sizeof(Recipe_t) + currentSendRecipeNumber * sizeof(RecipeEntry_t);
        u_char requestBuffer[sendSize];
        memcpy(requestBuffer, &requestBody, sizeof(requestBody));
        memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), &request, sizeof(Recipe_t));
        for (int i = 0; i < currentSendRecipeNumber; i++) {
            memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + sizeof(Recipe_t) + i * sizeof(RecipeEntry_t), &recipeList[sendRecipeNumber + i], sizeof(RecipeEntry_t));
        }

        u_char respondBuffer[sizeof(NetworkHeadStruct_t)];
        int recvSize = 0;
        if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
            cerr << "Sender : error sending file resipces, peer may close" << endl;
            return false;
        }
        memcpy(&respondBody, respondBuffer, sizeof(NetworkHeadStruct_t));
        status = respondBody.messageType;

        if (status == SUCCESS) {
            cerr << "Sender : send file reipce number = " << currentSendRecipeNumber << " done" << endl;
            sendRecipeNumber += currentSendRecipeNumber;
            currentSendRecipeNumber = 0;
        } else {
            cerr << "Sender : send file recipes fail" << endl;
            return false;
        }
        //cerr << "Sender : current remain recipe number = " << totalRecipeNumber << endl;
    }
    return true;
}

bool Sender::sendChunkList(char* requestBufferIn, int sendBufferSize, int sendChunkNumber, int& status)
{
    NetworkHeadStruct_t requestBody, respondBody;
    requestBody.clientID = clientID_;
    requestBody.messageType = CLIENT_UPLOAD_CHUNK;
    respondBody.clientID = 0;
    respondBody.messageType = 0;
    respondBody.dataSize = 0;
    u_char requestBuffer[NETWORK_MESSAGE_DATA_SIZE];
    int sendSize = sizeof(NetworkHeadStruct_t) + sizeof(int) + sendBufferSize;
    memcpy(requestBufferIn + sizeof(NetworkHeadStruct_t), &sendChunkNumber, sizeof(int));
    requestBody.dataSize = sendBufferSize + sizeof(int);
    memcpy(requestBufferIn, &requestBody, sizeof(NetworkHeadStruct_t));
    u_char respondBuffer[sizeof(NetworkHeadStruct_t)];
    int recvSize = 0;

    // gettimeofday(&timestartSenderRecipe, NULL);
    if (!this->sendData((u_char*)requestBufferIn, sendSize, respondBuffer, recvSize)) {
        return false;
    }
    // gettimeofday(&timeendSenderRecipe, NULL);
    // long diff = 1000000 * (timeendSenderRecipe.tv_sec - timestartSenderRecipe.tv_sec) + timeendSenderRecipe.tv_usec - timestartSenderRecipe.tv_usec;
    // double second = diff / 1000000.0;
    // printf("Sender send chunk list time is %ld us = %lf s\n", diff, second);

    memcpy(&respondBody, respondBuffer, sizeof(NetworkHeadStruct_t));
    status = respondBody.messageType;
    if (status == SUCCESS) {
        return true;
    } else {
        return false;
    }
}

bool Sender::sendSGXmsg01(uint32_t& msg0, sgx_ra_msg1_t& msg1, sgx_ra_msg2_t*& msg2, int& status)
{
    NetworkHeadStruct_t requestBody, respondBody;

    requestBody.clientID = clientID_;
    requestBody.messageType = SGX_RA_MSG01;
    respondBody.clientID = 0;
    respondBody.messageType = 0;
    respondBody.dataSize = 0;
    int sendSize = sizeof(NetworkHeadStruct_t) + sizeof(msg0) + sizeof(msg1);
    requestBody.dataSize = sizeof(msg0) + sizeof(msg1);
    u_char requestBuffer[sendSize];
    memcpy(requestBuffer, &requestBody, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), &msg0, sizeof(msg0));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + sizeof(msg0), &msg1, sizeof(msg1));

    u_char respondBuffer[SGX_MESSAGE_MAX_SIZE];
    int recvSize = 0;

    if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
        cerr << "Sender : peer closed, send sgx msg 01 error" << endl;
        return false;
    }
    memcpy(&respondBody, respondBuffer, sizeof(NetworkHeadStruct_t));
    status = respondBody.messageType;

    if (status == SUCCESS) {
        msg2 = (sgx_ra_msg2_t*)new char[recvSize - sizeof(NetworkHeadStruct_t)];
        memcpy(msg2, respondBuffer + sizeof(NetworkHeadStruct_t), recvSize - sizeof(NetworkHeadStruct_t));
        return true;
    }
    return false;
}

bool Sender::sendSGXmsg3(sgx_ra_msg3_t* msg3, uint32_t size, ra_msg4_t*& msg4, int& status)
{

    NetworkHeadStruct_t requestBody, respondBody;

    requestBody.clientID = clientID_;
    requestBody.messageType = SGX_RA_MSG3;
    respondBody.clientID = 0;
    respondBody.messageType = 0;
    respondBody.dataSize = 0;

    int sendSize = sizeof(NetworkHeadStruct_t) + size;
    requestBody.dataSize = size;
    u_char requestBuffer[sendSize];
    memcpy(requestBuffer, &requestBody, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), msg3, size);

    u_char respondBuffer[SGX_MESSAGE_MAX_SIZE];
    int recvSize = 0;

    if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
        cerr << "Sender : peer closed, send sgx msg 3 error" << endl;
        return false;
    }

    memcpy(&respondBody, respondBuffer, sizeof(NetworkHeadStruct_t));
    status = respondBody.messageType;

    if (status == SUCCESS) {
        msg4 = new ra_msg4_t;
        memcpy(msg4, respondBuffer + sizeof(NetworkHeadStruct_t), sizeof(ra_msg4_t));
        return true;
    }
    return false;
}

bool Sender::sendEnclaveSignedHash(powSignedHash_t& request, RequiredChunk_t& respond, int& status)
{
    NetworkHeadStruct_t requestBody, respondBody;
    requestBody.messageType = SGX_SIGNED_HASH;
    requestBody.clientID = clientID_;
    respondBody.messageType = 0;
    respondBody.clientID = 0;
    respondBody.dataSize = 0;

    int signedHashNumber = request.hash_.size();
    int sendSize = sizeof(NetworkHeadStruct_t) + sizeof(uint8_t) * 16 + signedHashNumber * CHUNK_HASH_SIZE;
    requestBody.dataSize = sizeof(uint8_t) * 16 + signedHashNumber * CHUNK_HASH_SIZE;
    u_char requestBuffer[sendSize];
    memcpy(requestBuffer, &requestBody, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), &request.signature_, 16 * sizeof(uint8_t));
    for (int i = 0; i < signedHashNumber; i++) {
        memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + 16 * sizeof(uint8_t) + i * CHUNK_HASH_SIZE, &request.hash_[i][0], CHUNK_HASH_SIZE);
    }

    u_char respondBuffer[SGX_MESSAGE_MAX_SIZE];
    int recvSize = 0;
    if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
        cerr << "Sender : send enclave signed hash to server & get back required chunk list" << endl;
        return false;
    }

    memcpy(&respondBody, respondBuffer, sizeof(NetworkHeadStruct_t));
    status = respondBody.messageType;
    int requiredChunkNumber;
    memcpy(&requiredChunkNumber, respondBuffer + sizeof(NetworkHeadStruct_t), sizeof(int));
    if (status == SUCCESS) {
        for (int i = 0; i < requiredChunkNumber; i++) {
            uint32_t tempID;
            memcpy(&tempID, respondBuffer + sizeof(NetworkHeadStruct_t) + sizeof(int) + i * sizeof(uint32_t), sizeof(uint32_t));
            respond.push_back(tempID);
        }
        return true;
    } else {
        return false;
    }
}

bool Sender::sendData(u_char* request, int requestSize, u_char* respond, int& respondSize)
{
    std::lock_guard<std::mutex> locker(mutexSocket_);
    // gettimeofday(&timestartSenderRecipe, NULL);
    if (!socket_.Send(request, requestSize)) {
        cerr << "Sender : send data error peer closed" << endl;
        return false;
    }
    // gettimeofday(&timeendSenderRecipe, NULL);
    // long diff = 1000000 * (timeendSenderRecipe.tv_sec - timestartSenderRecipe.tv_sec) + timeendSenderRecipe.tv_usec - timestartSenderRecipe.tv_usec;
    // double second = diff / 1000000.0;
    // printf("Sender : send data time is %ld us = %lf s\n", diff, second);

    // gettimeofday(&timestartSenderRecipe, NULL);
    if (!socket_.Recv(respond, respondSize)) {
        cerr << "Sender : recv data error peer closed" << endl;
        return false;
    }
    // gettimeofday(&timeendSenderRecipe, NULL);
    // diff = 1000000 * (timeendSenderRecipe.tv_sec - timestartSenderRecipe.tv_sec) + timeendSenderRecipe.tv_usec - timestartSenderRecipe.tv_usec;
    // second = diff / 1000000.0;
    // printf("Sender : recv data time is %ld us = %lf s\n", diff, second);

    return true;
}

void Sender::run()
{
    gettimeofday(&timestartSender, NULL);
    Data_t tempChunk;
    RecipeList_t recipeList;
    Recipe_t fileRecipe;
    int sendBatchSize = config.getSendChunkBatchSize();
    int status;
    char* sendChunkBatchBuffer = (char*)malloc(sizeof(NetworkHeadStruct_t) + sizeof(int) + sizeof(char) * sendBatchSize * (CHUNK_HASH_SIZE + MAX_CHUNK_SIZE + sizeof(int)));
    bool jobDoneFlag = false;
    int currentChunkNumber = 0;
    int currentSendRecipeNumber = 0;
    int currentSendChunkBatchBufferSize = sizeof(NetworkHeadStruct_t) + sizeof(int);
    while (!jobDoneFlag) {
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            jobDoneFlag = true;
        }
        if (extractMQFromPow(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                // cerr << "Sender : get file recipe head, file size = " << tempChunk.recipe.fileRecipeHead.fileSize << " file chunk number = " << tempChunk.recipe.fileRecipeHead.totalChunkNumber << endl;
                // PRINT_BYTE_ARRAY_SENDER(stderr, tempChunk.recipe.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
                memcpy(&fileRecipe, &tempChunk.recipe, sizeof(Recipe_t));
                continue;
            } else {
                if (tempChunk.chunk.type == CHUNK_TYPE_NEED_UPLOAD) {
                    memcpy(sendChunkBatchBuffer + currentSendChunkBatchBufferSize, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                    currentSendChunkBatchBufferSize += CHUNK_HASH_SIZE;
                    memcpy(sendChunkBatchBuffer + currentSendChunkBatchBufferSize, &tempChunk.chunk.logicDataSize, sizeof(int));
                    currentSendChunkBatchBufferSize += sizeof(int);
                    memcpy(sendChunkBatchBuffer + currentSendChunkBatchBufferSize, tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize);
                    currentSendChunkBatchBufferSize += tempChunk.chunk.logicDataSize;
                    currentChunkNumber++;
                    // cout << "Sender : Chunk ID = " << tempChunk.chunk.ID << " size = " << tempChunk.chunk.logicDataSize << endl;
                    // PRINT_BYTE_ARRAY_SENDER(stdout, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                    // PRINT_BYTE_ARRAY_SENDER(stdout, tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                    // PRINT_BYTE_ARRAY_SENDER(stdout, tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize);
                }

                RecipeEntry_t newRecipeEntry;
                newRecipeEntry.chunkID = tempChunk.chunk.ID;
                newRecipeEntry.chunkSize = tempChunk.chunk.logicDataSize;
                memcpy(newRecipeEntry.chunkHash, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                memcpy(newRecipeEntry.chunkKey, tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                recipeList.push_back(newRecipeEntry);
                currentSendRecipeNumber++;
            }
        }
        if (currentChunkNumber == sendBatchSize || jobDoneFlag) {
            // cerr << "Sender : run -> start send " << setbase(10) << currentChunkNumber << " chunks to server, size = " << setbase(10) << currentSendChunkBatchBufferSize << endl;
            while (true) {
                // gettimeofday(&timestartSenderRecipe, NULL);
                this->sendChunkList(sendChunkBatchBuffer, currentSendChunkBatchBufferSize, currentChunkNumber, status);
                // gettimeofday(&timeendSenderRecipe, NULL);
                // long diff = 1000000 * (timeendSenderRecipe.tv_sec - timestartSenderRecipe.tv_sec) + timeendSenderRecipe.tv_usec - timestartSenderRecipe.tv_usec;
                // double second = diff / 1000000.0;
                // printf("Sender send chunk list time is %ld us = %lf s\n", diff, second);

                if (status == SUCCESS) {
                    cerr << "Sender : sent " << setbase(10) << currentChunkNumber << " chunk" << endl;
                    currentSendChunkBatchBufferSize = sizeof(NetworkHeadStruct_t) + sizeof(int);
                    currentChunkNumber = 0;
                    break;
                }
                if (status == ERROR_CLOSE) {
                    std::cerr << "Sender : Server Reject Chunk and send close flag" << endl;
                    return;
                }
                if (status == ERROR_RESEND) {
                    std::cerr << "Sender : Server Reject Chunk and send resend flag" << endl;
                    continue;
                }
            }
        }
    }

    while (true) {
        gettimeofday(&timestartSenderRecipe, NULL);
        this->sendRecipe(fileRecipe, recipeList, status);
        gettimeofday(&timeendSenderRecipe, NULL);
        long diff = 1000000 * (timeendSenderRecipe.tv_sec - timestartSenderRecipe.tv_sec) + timeendSenderRecipe.tv_usec - timestartSenderRecipe.tv_usec;
        double second = diff / 1000000.0;
        printf("Sender send recipe list time is %ld us = %lf s\n", diff, second);
        if (status == SUCCESS) {
            std::cerr << "Sender : send recipe success" << endl;
            break;
        }
        if (status == ERROR_CLOSE) {
            std::cerr << "Sender : Server Reject Chunk and send close flag" << endl;
            exit(1);
        }
        if (status == ERROR_RESEND) {
            std::cerr << "Sender : Server Reject Chunk and send resend flag" << endl;
            continue;
        }
    }
    free(sendChunkBatchBuffer);

    gettimeofday(&timeendSender, NULL);
    long diff = 1000000 * (timeendSender.tv_sec - timestartSender.tv_sec) + timeendSender.tv_usec - timestartSender.tv_usec;
    double second = diff / 1000000.0;
    printf("Sender thread work time is %ld us = %lf s\n", diff, second);
    return;
}

bool Sender::insertMQFromPow(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool Sender::extractMQFromPow(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool Sender::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}
