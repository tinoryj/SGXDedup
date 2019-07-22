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
    messageQueue<Chunk_t> inputMQ_;
    encoder* encoderObj_;
    CryptoPrimitive* cryptoObj_;
    int keyBatchSizeMin_, keyBatchSizeMax_;
    Socket socket_;
    bool trustdKM_;

public:
    keyClient(encoder* encoderObjTemp);
    ~keyClient();
    void run();
    bool insertMQFromChunker(Chunk_t newChunk);
    bool extractMQFromChunker(Chunk_t newChunk);
    bool insertMQtoEncoder(Chunk_t newChunk);
    bool editJobDoneFlag();
    string keyExchange(Chunk_t champion);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
