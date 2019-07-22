//
// Created by a on 1/12/19.
//

#include "sender.hpp"

extern Configure config;

Sender::Sender()
{
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

bool Sender::sendRecipeList(RecipeList_t& request, int& status)
{
    NetworkStruct_t requestBody, respondBody;
    string requestBuffer, respondBuffer;
    requestBody.clientID = config.getClientID();
    requestBody.messageType = CLIENT_UPLOAD_RECIPE;
    respondBody.clientID = 0;
    respondBody.messageType = 0;

    serialize(request, requestBody.data);
    serialize(requestBody, requestBuffer);

    if (!this->sendData(requestBuffer, respondBuffer)) {
        return false;
    }

    deserialize(respondBuffer, respondBody);
    status = respondBody.messageType;

    if (status == SUCCESS) {
        return true;
    } else {
        return false;
    }
}

bool Sender::sendChunkList(ChunkList_t& request, int& status)
{

    NetworkStruct_t requestBody, respondBody;
    string requestBuffer, respondBuffer;
    requestBody.clientID = config.getClientID();
    requestBody.messageType = CLIENT_UPLOAD_CHUNK;
    respondBody.clientID = 0;
    respondBody.messageType = 0;

    serialize(request, requestBody.data);
    serialize(requestBody, requestBuffer);

    if (!this->sendData(requestBuffer, respondBuffer)) {
        return false;
    }

    deserialize(respondBuffer, respondBody);
    status = respondBody.messageType;

    if (status == SUCCESS)
        return true;
    return false;
}

bool Sender::sendSGXmsg01(uint32_t& msg0, sgx_ra_msg1_t& msg1, sgx_ra_msg2_t*& msg2, int& status)
{

    NetworkStruct_t requestBody, respondBody;
    string requestBuffer, respondBuffer;
    requestBody.clientID = config.getClientID();
    requestBody.messageType = SGX_RA_MSG01;
    respondBody.clientID = 0;
    respondBody.messageType = 0;

    string& requestBuffer = requestBody.data;
    requestBuffer.resize(sizeof(msg0) + sizeof(msg1));
    memcpy(&requestBuffer[0], &msg0, sizeof(char));
    memcpy(&requestBuffer[sizeof(msg0)], &msg1, sizeof(msg1));
    string sendBuffer, recvBuffer;
    serialize(requestBody, sendBuffer);
    if (!this->sendData(sendBuffer, recvBuffer)) {
        cerr << "Sender : peer closed" << endl;
        return false;
    }
    deserialize(recvBuffer, respondBody);
    status = respondBody.messageType;
    if (status == SUCCESS) {
        msg2 = (sgx_ra_msg2_t*)new char[respondBody.data.length()];
        memcpy(msg2, &respondBody.data[0], respondBody.data.length());
        return true;
    }
    return false;
}

bool Sender::sendSGXmsg3(sgx_ra_msg3_t& msg3, uint32_t sz, ra_msg4_t*& msg4, int& status)
{

    NetworkStruct_t requestBody, respondBody;
    string requestBuffer, respondBuffer;
    requestBody.clientID = config.getClientID();
    requestBody.messageType = SGX_RA_MSG3;
    respondBody.clientID = 0;
    respondBody.messageType = 0;

    requestBody.data.resize(sz);

    memcpy(&requestBody.data[0], &msg3, sz);
    serialize(requestBody, requestBuffer);

    this->sendData(requestBuffer, respondBuffer);

    deserialize(respondBuffer, respondBody);
    status = respondBody.messageType;

    if (status == SUCCESS) {
        msg4 = new ra_msg4_t;
        memcpy(msg4, &respondBuffer[0], sizeof(ra_msg4_t));
        return true;
    }
    return false;
}

bool Sender::sendEnclaveSignedHash(powSignedHash& request, RequiredChunk& respond, int& status)
{
    NetworkStruct_t requestBody, respondBody;
    requestBody.messageType = SGX_SIGNED_HASH;
    requestBody.clientID = config.getClientID();
    respondBody.messageType = 0;
    respondBody.clientID = 0;

    string requestBuffer, respondBuffer;

    serialize(request, requestBody.data);
    serialize(requestBody, requestBuffer);

    if (!this->sendData(requestBuffer, respondBuffer)) {
        cerr << "Sender : peer closed" << endl;
        return false;
    }

    deserialize(respondBuffer, respondBody);
    status = respondBody.messageType;

    if (status == SUCCESS) {
        deserialize(respondBody.data, respond);
        return true;
    }

    return false;
}

bool Sender::sendData(string& request, string& respond)
{
    std::lock_guard<std::mutex> locker(mutexSocket_);
    if (!socket_.Send(request)) {
        cerr << "Sender : peer closed" << endl;

        exit(0);
    }
    if (!socket_.Recv(respond)) {
        cerr << "Sender : peer closed" << endl;
        exit(0);
    }
    return true;
}

void Sender::run()
{
    ChunkList_t chunkList;
    Chunk_t tempChunk;
    RecipeList_t recipeList;

    int status;

    bool start = false;

    while (true) {
        chunkList.clear();
        recipeList.clear();
        for (int i = 0; i < config.getSendChunkBatchSize(); i++) {
            if (inputMQ_.done_ && !extractMQFromPow(tempChunk)) {
                if (start) {
                    cerr << "Sender : sending chunks and recipes end" << endl;
                    start = false;
                }
                break;
            }

            if (!start) {
                cerr << "Sender : start sending chunks and recipes" << endl;
                start = true;
            }
            if (tempChunk.type == CHUNK_TYPE_NEED_UPLOAD) {
                chunkList.push_back(tempChunk);
            }

            RecipeEntry_t newRecipeEntry;
            newRecipeEntry.chunkID = tempChunk.ID;
            newRecipeEntry.chunkSize = tempChunk.logicDataSize;
            memcpy(newRecipeEntry.chunkHash, tempChunk.chunkHash, CHUNK_HASH_SIZE);
            memcpy(newRecipeEntry.chunkKey, tempChunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
            recipeList.push_back(newRecipeEntry);
        }

        if (chunkList.empty()) {
            continue;
        }

        bool success = false;
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

            this->sendRecipeList(recipeList, status);
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
        if (!start) {
            break;
        }
    }
    pthread_exit(NULL);
}

bool Sender::insertMQFromPow(Chunk_t newChunk)
{
    return inputMQ_.push(newChunk);
}

bool Sender::extractMQFromPow(Chunk_t newChunk)
{
    return inputMQ_.pop(newChunk);
}

bool Sender::editJobDoneFlag()
{
    inputMQ_.done_ = true;
    if (inputMQ_.done_) {
        return true;
    } else {
        return false;
    }
}
