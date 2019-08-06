#include "sender.hpp"

extern Configure config;

Sender::Sender()
{
    inputMQ_ = new messageQueue<Data_t>(3000);
    socket_.init(CLIENT_TCP, config.getStorageServerIP(), config.getStorageServerPort());
    cryptoObj_ = new CryptoPrimitive();
}

Sender::~Sender()
{
    socket_.finish();
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}

bool Sender::sendRecipe(Recipe_t& request, RecipeList_t& recipeList, int& status)
{
    int totalRecipeNumber = recipeList.size();
    int sendRecipeNumber = 0;
    int currentRecipeNumber = 0;
    while (totalRecipeNumber != 0) {

        if (totalRecipeNumber - sendRecipeNumber < config.getSendRecipeBatchSize()) {
            currentRecipeNumber = totalRecipeNumber - sendRecipeNumber;
        } else {
            currentRecipeNumber = config.getSendRecipeBatchSize();
        }
        NetworkHeadStruct_t requestBody, respondBody;

        requestBody.clientID = config.getClientID();
        requestBody.messageType = CLIENT_UPLOAD_RECIPE;
        respondBody.clientID = 0;
        respondBody.messageType = 0;
        respondBody.dataSize = 0;
        int sendSize = sizeof(NetworkHeadStruct_t) + sizeof(Recipe_t) + currentRecipeNumber * sizeof(RecipeEntry_t);
        requestBody.dataSize = sizeof(Recipe_t) + currentRecipeNumber * sizeof(RecipeEntry_t);
        u_char requestBuffer[sendSize];
        memcpy(requestBuffer, &requestBody, sizeof(requestBody));
        memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), &request, sizeof(Recipe_t));
        for (int i = 0; i < currentRecipeNumber; i++) {
            memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + sizeof(Recipe_t) + i * sizeof(RecipeEntry_t), &recipeList[sendRecipeNumber + i], sizeof(RecipeEntry_t));
        }

        u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
        int recvSize = 0;
        if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
            return false;
        }
        memcpy(&respondBody, respondBuffer, sizeof(NetworkHeadStruct_t));
        status = respondBody.messageType;

        if (status == SUCCESS) {
            sendRecipeNumber += currentRecipeNumber;
            currentRecipeNumber = 0;
            totalRecipeNumber -= currentRecipeNumber;
        } else {
            return false;
        }
    }
    return true;
}

bool Sender::sendChunkList(ChunkList_t& request, int& status)
{
    NetworkHeadStruct_t requestBody, respondBody;

    requestBody.clientID = config.getClientID();
    requestBody.messageType = CLIENT_UPLOAD_CHUNK;
    respondBody.clientID = 0;
    respondBody.messageType = 0;
    respondBody.dataSize = 0;
    int chunkNumber = request.size();
    u_char requestBuffer[EPOLL_MESSAGE_DATA_SIZE];
    int sendSize = sizeof(NetworkHeadStruct_t) + sizeof(int);
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), &chunkNumber, sizeof(int));
    for (int i = 0; i < chunkNumber; i++) {
        memcpy(requestBuffer + sendSize, request[i].chunkHash, CHUNK_HASH_SIZE);
        sendSize += CHUNK_HASH_SIZE;
        memcpy(requestBuffer + sendSize, &request[i].logicDataSize, sizeof(int));
        sendSize += sizeof(int);
        memcpy(requestBuffer + sendSize, request[i].logicData, request[i].logicDataSize);
        sendSize += request[i].logicDataSize;
    }
    requestBody.dataSize = sendSize - sizeof(NetworkHeadStruct_t);
    memcpy(requestBuffer, &requestBody, sizeof(NetworkHeadStruct_t));

    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize = 0;

    if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
        return false;
    }

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

    requestBody.clientID = config.getClientID();
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

    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
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

    requestBody.clientID = config.getClientID();
    requestBody.messageType = SGX_RA_MSG3;
    respondBody.clientID = 0;
    respondBody.messageType = 0;
    respondBody.dataSize = 0;

    int sendSize = sizeof(NetworkHeadStruct_t) + size;
    requestBody.dataSize = size;
    u_char requestBuffer[sendSize];
    memcpy(requestBuffer, &requestBody, sizeof(NetworkHeadStruct_t));
    memcpy(requestBuffer + sizeof(NetworkHeadStruct_t), msg3, size);

    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
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
    requestBody.clientID = config.getClientID();
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
        memcpy(requestBuffer + sizeof(NetworkHeadStruct_t) + 16 * sizeof(uint8_t), &request.hash_[i][0], CHUNK_HASH_SIZE);
    }

    u_char respondBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize = 0;
    if (!this->sendData(requestBuffer, sendSize, respondBuffer, recvSize)) {
        cerr << "Sender : peer closed" << endl;
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
    if (!socket_.Send(request, requestSize)) {
        cerr << "Sender : peer closed" << endl;
        exit(0);
    }
    if (!socket_.Recv(respond, respondSize)) {
        cerr << "Sender : peer closed" << endl;
        exit(0);
    }
    return true;
}

void Sender::run()
{
    ChunkList_t chunkList;
    Data_t tempChunk;
    RecipeList_t recipeList;
    Recipe_t fileRecipe;

    int status;

    bool jobDoneFlag = false;

    while (true) {
        for (int i = 0; i < config.getSendChunkBatchSize(); i++) {
            if (inputMQ_->done_ && !extractMQFromPow(tempChunk)) {
                jobDoneFlag = true;
                break;
            }
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                fileRecipe = tempChunk.recipe;
                continue;
            }
            if (tempChunk.chunk.type == CHUNK_TYPE_NEED_UPLOAD) {
                chunkList.push_back(tempChunk.chunk);
            }
            RecipeEntry_t newRecipeEntry;
            newRecipeEntry.chunkID = tempChunk.chunk.ID;
            newRecipeEntry.chunkSize = tempChunk.chunk.logicDataSize;
            memcpy(newRecipeEntry.chunkHash, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            memcpy(newRecipeEntry.chunkKey, tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
            recipeList.push_back(newRecipeEntry);
        }

        while (true) {
            this->sendChunkList(chunkList, status);
            if (status == SUCCESS) {
                cerr << "Sender : sent " << setbase(10) << chunkList.size() << " chunk" << endl;
                break;
            }
            if (status == ERROR_CLOSE) {
                std::cerr << "Sender : Server Reject Chunk and send close flag" << endl;
                exit(1);
            }
            if (status == ERROR_RESEND) {
                std::cerr << "Sender : Server Reject Chunk and send resend flag" << endl;
            }
            chunkList.clear();
        }

        if (jobDoneFlag == true) {
            while (true) {
                this->sendRecipe(fileRecipe, recipeList, status);
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
            recipeList.clear();
        }
        if (jobDoneFlag == true) {
            break;
        }
    }
    pthread_exit(0);
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
