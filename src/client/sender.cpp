//
// Created by a on 1/12/19.
//

#include "sender.hpp"

extern Configure config;

Sender::Sender()
{
    socket.init(CLIENT_TCP, config.getStorageServerIP(), config.getStorageServerPort());
    cryptoObj = new CryptoPrimitive();
}
Sender::~Sender()
{
    socket.finish();
    if (cryptoObj != NULL) {
        delete cryptoObj;
    }
}

bool Sender::sendRecipe(Recipe_t& request, int& status)
{
    NetworkStruct_t requestBody, respondBody;
    string requestBuffer, respondBuffer;
    requestBody.clientID = config.getClientID();
    requestBody.messageType = CLIENT_UPLOAD_RECIPE;
    respondBody.clientID = 0;
    respondBody.messageType = 0;

    /**********************/
    //TODO:temp implement
    char recipekey[128];
    memset(recipekey, 0, sizeof(recipekey));

    cryptoObj->setSymKey(recipekey, 128, recipekey, 128);
    cryptoObj->recipe_encrypt(request._k, request._kencrypted);

    request._k._body.clear();
    /*********************/

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
    std::lock_guard<std::mutex> locker(mutexSocket);
    if (!socket.Send(request)) {
        cerr << "Sender : peer closed" << endl;

        exit(0);
    }
    if (!socket.Recv(respond)) {
        cerr << "Sender : peer closed" << endl;
        exit(0);
    }
    return true;
}

void Sender::run()
{
    ChunkList_t chunkList;
    Chunk_t tempChunk;
    Recipe_t recipe;
    int status;

    bool start = false;

    while (true) {
        chunkList.clear();
        for (int i = 0; i < config.getSendChunkBatchSize(); i++) {
            if (inputMQ.done_ && !extractMQFromPow(tempChunk)) {
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

            chunkList.push_back(tempChunk);
            recipe = tempChunk.getRecipePointer();
            recipe->_chunkCnt--;
            string hash;
            hash.resize(CHUNK_HASH_SIZE);
            memcpy(&hash[0], tempChunk.chunkHash, CHUNK_HASH_SIZE);
            memcpy(recipe->_f._body[tempChunk.getID()]._chunkHash, hash.c_str(), 32);
            memcpy(recipe->_k._body[tempChunk.getID()]._chunkHash, hash.c_str(), 32);

            if (recipe->_chunkCnt == 0) {
                recipe->_chunkCnt--;
                while (1) {
                    this->sendRecipe(*recipe, status);
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
                delete &recipe;
            }
        }

        if (chunkList._chunkList.empty()) {
            continue;
        }

        bool success = false;
        while (1) {
            success = this->sendChunkList(chunkList, status);
            if (success) {
                cerr << "Sender : sent " << setbase(10) << chunkList._FP.size() << " chunk" << endl;
                break;
            }
            if (status == ERROR_CLOSE) {
                std::cerr << "Sender : Server Reject Chunk and send close flag" << endl;
                exit(1);
            }
            if (status == ERROR_RESEND) {
                std::cerr << "Sender : Server Reject Chunk and send resend flag" << endl;
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
    return inputMQ.push(newChunk);
}

bool Sender::extractMQFromPow(Chunk_t newChunk)
{
    return inputMQ.pop(newChunk);
}

bool Sender::editJobDoneFlag()
{
    inputMQ.done_ = true;
    if (inputMQ.done_) {
        return true;
    } else {
        return false;
    }
}
