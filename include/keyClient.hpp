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
    messageQueue<Data_t>* inputMQ_;
    Encoder* encoderObj_;
    CryptoPrimitive* cryptoObj_;
    int keyBatchSize_;
    Socket socket_;
    bool trustdKM_;
    u_char keyExchangeKey_[16];

public:
    keyClient(Encoder* encoderObjTemp);
    ~keyClient();
    void run();
    bool insertMQFromChunker(Data_t& newChunk);
    bool extractMQFromChunker(Data_t& newChunk);
    bool insertMQtoEncoder(Data_t& newChunk);
    bool editJobDoneFlag();
    bool setJobDoneFlag();
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
