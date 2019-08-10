#ifndef GENERALDEDUPSYSTEM_KEYCLIENT_HPP
#define GENERALDEDUPSYSTEM_KEYCLIENT_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "kmServer.hpp"
#include "messageQueue.hpp"
#include "powClient.hpp"
#include "powSession.hpp"
#include "socket.hpp"

#define KEYMANGER_PUBLIC_KEY_FILE "key/serverpub.key"

class keyClient {
private:
    messageQueue<Data_t>* inputMQ_;
    powClient* powObj_;
    CryptoPrimitive* cryptoObj_;
    int keyBatchSize_;
    Socket socket_;
    bool trustdKM_;
    u_char keyExchangeKey_[16];

public:
    keyClient(powClient* powObjTemp);
    ~keyClient();
    void run();
    bool encodeChunk(Data_t& newChunk);
    bool insertMQFromChunker(Data_t& newChunk);
    bool extractMQFromChunker(Data_t& newChunk);
    bool insertMQToPOW(Data_t& newChunk);
    bool editJobDoneFlag();
    bool setJobDoneFlag();
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
