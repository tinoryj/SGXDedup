//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYCLIENT_HPP
#define GENERALDEDUPSYSTEM_KEYCLIENT_HPP

#include "cache.hpp"
#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "encoder.hpp"
#include "kmServer.hpp"
#include "messageQueue.hpp"
#include "powSession.hpp"
#include "socket.hpp"

#define KEYMANGER_PUBLIC_KEY_FILE "key/serverpub.key"

class keyClient {
private:
    messageQueue<Chunk_t> inputMQ;
    encoder* encoderObj;
    CryptoPrimitive* cryptoObj;
    int keyBatchSizeMin, keyBatchSizeMax;
    Socket socket;
    bool trustdKM;

public:
    keyClient();
    ~keyClient();
    void run();
    bool insertMQFromChunker(Chunk_t newChunk);
    bool extractMQFromChunker(Chunk_t newChunk);
    bool insertMQtoEncoder(Chunk_t newChunk);
    bool editJobDoneFlag();
    string keyExchange(Chunk_t champion);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
