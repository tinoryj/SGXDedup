//
// Created by a on 1/12/19.
//

#include "sender.hpp"

extern Configure config;


bool Sender::sendRecipe(Recipe &request, int &status) {
    static networkStruct requestBody(CLIENT_UPLOAD_RECIPE, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0, 0);

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    this->sendData(requestBuffer, respondBuffer);

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;

    if (status == OK) return true;
    return false;
}

bool Sender::sendChunkList(chunkList &request, int &status) {
    static networkStruct requestBody(CLIENT_UPLOAD_CHUNK, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0,0);

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    this->sendData(requestBuffer, respondBuffer);

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;

    if (status == OK) return true;
    return false;
}

bool Sender::sendSignedHashList(powSignedHash &request, RequiredChunk &respond, int status) {
    static networkStruct requestBody(SGX_SIGNED_HASH, config.getClientID());
    static string requestBuffer, respondBuffer;
    networkStruct respondBody(0, 0);

    serialize(request, requestBody._data);
    serialize(requestBody, requestBuffer);

    this->sendData(requestBuffer, respondBuffer);

    deserialize(respondBuffer, respondBody);
    status = respondBody._type;


    if (status == OK) {
        deserialize(respondBody._data, respond);
        return true;
    }

    return false;
}

bool Sender::sendData(string &request, string &respond) {
    {
        boost::unique_lock<boost::shared_mutex> t(this->_sockMtx);
        _socket.Send(request);
        _socket.Recv(respond);
    }
}



void Sender::run() {
    chunkList chunks;
    Chunk tmpChunk;
    while (1) {
        extractMQ(tmpChunk);
        chunks._chunks.push_back(tmpChunk.getLogicData());
        chunks._FP.push_back(tmpChunk.getChunkHash());
        if (chunks._FP.size() == config.getChunkBatchSize()) {
            int status;
            bool success = false;
            while (1) {
                success = this->sendChunkList(chunks, status);
                if (success)break;
                if (status == ERROR_CLOSE) {
                    std::cerr << "Server Reject Chunk and send close flag\n";
                    exit(1);
                }
            }
            chunks._FP.resize(0);
            chunks._chunks.resize(0);
        }
    }
}