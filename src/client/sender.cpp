//
// Created by a on 1/12/19.
//

#include "sender.hpp"


extern Configure config;

Sender::Sender() {
    _socket.init(CLIENTTCP,config.getStorageServerIP(0),config.getStorageServerPort(0));
}
Sender::~Sender() {
    _socket.finish();
}

bool Sender::sendRecipe(Recipe_t &request, int &status) {
    static networkStruct requestBody(CLIENT_UPLOAD_RECIPE, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0, 0);

    /**********************/
    //TODO:temp implement
    char recipekey[128];
    memset(recipekey, 0, sizeof(recipekey));
    CryptoPrimitive *_crypto=new CryptoPrimitive();
    _crypto->setSymKey(recipekey, 128, recipekey, 128);
    _crypto->recipe_encrypt(request._k,request._kencrypted);
    delete _crypto;
    request._k._body.clear();
    /*********************/

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    if(!this->sendData(requestBuffer, respondBuffer)){
        return false;
    }

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;

    if (status == SUCCESS) return true;
    return false;
}

bool Sender::sendChunkList(chunkList &request, int &status) {
    static networkStruct requestBody(CLIENT_UPLOAD_CHUNK, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0,0);

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    if(!this->sendData(requestBuffer, respondBuffer)){
        return false;
    }

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;

    if (status == SUCCESS) return true;
    return false;
}

bool Sender::sendSGXmsg01(uint32_t &msg0, sgx_ra_msg1_t &msg1, sgx_ra_msg2_t *&msg2, int &status) {
    networkStruct requestBody(SGX_RA_MSG01, config.getClientID());
    networkStruct respondBody(0, 0);
    string &requestBuffer = requestBody._data;
    requestBuffer.resize(sizeof(msg0) + sizeof(msg1));
    memcpy(&requestBuffer[0], &msg0, sizeof(char));
    memcpy(&requestBuffer[sizeof(msg0)], &msg1, sizeof(msg1));
    string sendBuffer, recvBuffer;
    serialize(requestBody, sendBuffer);
    if (!this->sendData(sendBuffer, recvBuffer)) {
        cerr << "Sender : peer closed\n";
        return false;
    }
    deserialize(recvBuffer, respondBody);
    status = respondBody._type;
    if (status == SUCCESS) {
        msg2 = (sgx_ra_msg2_t *) new char[respondBody._data.length()];
        memcpy(msg2, &respondBody._data[0], respondBody._data.length());
        return true;
    }
    return false;
}

bool Sender::sendSGXmsg3(sgx_ra_msg3_t &msg3, uint32_t sz, ra_msg4_t *&msg4, int &status) {
    networkStruct requestBody(SGX_RA_MSG3, config.getClientID());
    networkStruct respondBody(0, 0);
    string requestBuffer, respondBuffer;

    requestBody._data.resize(sz);
    memcpy(&requestBody._data[0], &msg3, sz);
    serialize(requestBody, requestBuffer);

    this->sendData(requestBuffer, respondBuffer);

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;

    if (status == SUCCESS) {
        msg4 = new ra_msg4_t;
        memcpy(msg4, &respondBuffer[0], sizeof(ra_msg4_t));
        return true;
    }
    return false;
}

bool Sender::sendEnclaveSignedHash(powSignedHash &request, RequiredChunk &respond, int &status) {
    static networkStruct requestBody(SGX_SIGNED_HASH, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0, 0);

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    if(!this->sendData(requestBuffer, respondBuffer)){
        cerr<<"Sender : peer closed\n";
        return false;
    }

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;


    if (status == SUCCESS) {
        deserialize(respondBody._data, respond);
        return true;
    }

    return false;
}

bool Sender::sendData(string &request, string &respond) {
    {
        boost::unique_lock<boost::shared_mutex> t(this->_sockMtx);
        if(!_socket.Send(request)){
            cerr<<"Sender : peer closed\n";
            // return false;
            exit(0);
        }
        if(!_socket.Recv(respond)){
            cerr<<"Sender : peer closed\n";
            exit(0);
            // return false;
        }
    }
    return true;
}



void Sender::run() {
    chunkList chunks;
    Chunk tmpChunk;
    Recipe_t *recipe;
    int status;

    bool start = false;

    while (1) {
        chunks.clear();
        for (int i = 0; i < config.getSendChunkBatchSize(); i++) {
            if (!extractMQ(tmpChunk)) {
                if (start) {
                    cerr << "Sender : end\n";
                    start = false;
                }
                break;
            }

            if (!start) {
                cerr << "Sender : start\n";
                start = true;
            }

            chunks.push_back(tmpChunk);
            recipe = tmpChunk.getRecipePointer();
            recipe->_chunkCnt--;
            string hash = tmpChunk.getChunkHash();
            memcpy(recipe->_f._body[tmpChunk.getID()]._chunkHash, hash.c_str(), 32);
            memcpy(recipe->_k._body[tmpChunk.getID()]._chunkHash, hash.c_str(), 32);

            if (recipe->_chunkCnt==0) {
                recipe->_chunkCnt--;
                while (1) {
                    this->sendRecipe(*recipe, status);
                    if (status == SUCCESS) {
                        std::cerr << "Sender : send recipe success\n";
                        break;
                    }
                    if (status == ERROR_CLOSE) {
                        std::cerr << "Sender : Server Reject Chunk and send close flag\n";
                        exit(1);
                    }
                    if (status == ERROR_RESEND) {
                        std::cerr << "Sender : Server Reject Chunk and send resend flag\n";
                        continue;
                    }
                }
                delete recipe;
            }
        }

        if (chunks._chunks.empty()) {
            continue;
        }

        bool success = false;
        while (1) {
            success = this->sendChunkList(chunks, status);
            if (success) {
                cerr << "Sender : " << "sent " << setbase(10) << chunks._FP.size() << " chunk\n";
                break;
            }
            if (status == ERROR_CLOSE) {
                std::cerr << "Sender : Server Reject Chunk and send close flag\n";
                exit(1);
            }
            if (status == ERROR_RESEND) {
                std::cerr << "Sender : Server Reject Chunk and send resend flag\n";
            }
        }
    }
}