//
// Created by a on 1/12/19.
//

#include "sender.hpp"

extern Configure config;

Sender::Sender() {
    _socket.init(CLIENTTCP,config.getStorageServerIP(),config.getStorageServerPort());
}
Sender::~Sender() {
    _socket.finish();
}

bool Sender::sendRecipe(Recipe_t &request, int &status) {
    static networkStruct requestBody(CLIENT_UPLOAD_RECIPE, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0, 0);

    /**********************/
    //temp implement
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

bool Sender::sendSignedHashList(powSignedHash &request, RequiredChunk &respond, int status) {
    static networkStruct requestBody(SGX_SIGNED_HASH, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0, 0);

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    if(!this->sendData(requestBuffer, respondBuffer)){
        cerr<<"peer closed\n";
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
            cerr<<"peer closed\n";
            return false;
        }
        if(!_socket.Recv(respond)){
            cerr<<"peer closed\n";
            return false;
        }
    }
}



void Sender::run() {
    chunkList chunks;
    Chunk tmpChunk;
    Recipe_t *recipe;
    int status;

    while (1) {
        chunks.clear();
        for (int i = 0; i < config.getSendChunkBatchSize(); i++) {
            if (!extractMQ(tmpChunk)) {
                break;
            }
            recipe = tmpChunk._recipe;
            recipe->_k._fileSize -= tmpChunk.getLogicDataSize();
            chunks.push_back(tmpChunk);
            if (recipe->_k._fileSize == 0) {
                recipe->_k._fileSize = recipe->_f._fileSize;
                while (1) {
                    this->sendRecipe(*recipe, status);
                    if (status == SUCCESS) {
                        break;
                    }
                    if (status == ERROR_CLOSE) {
                        std::cerr << "Server Reject Chunk and send close flag\n";
                        exit(1);
                    }
                    if (status == ERROR_RESEND) {
                        continue;
                    }
                }
                delete recipe;
            }
        }

        if(chunks._chunks.empty()){
            continue;
        }

        bool success = false;
        while (1) {
            success = this->sendChunkList(chunks, status);
            if (success)break;
            if (status == ERROR_CLOSE) {
                std::cerr << "Server Reject Chunk and send close flag\n";
                exit(1);
            }
        }
    }
}