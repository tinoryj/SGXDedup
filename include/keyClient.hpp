#ifndef GENERALDEDUPSYSTEM_KEYCLIENT_HPP
#define GENERALDEDUPSYSTEM_KEYCLIENT_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "encoder.hpp"
#include "messageQueue.hpp"
#include "ssl.hpp"

class keyClient {
private:
    messageQueue<Data_t>* inputMQ_;
    Encoder* encoderObj_;
    CryptoPrimitive* cryptoObj_;
    int keyBatchSize_;
    ssl* keySecurityChannel_;
    SSL* sslConnection_;
    u_char keyExchangeKey_[KEY_SERVER_SESSION_KEY_SIZE];

public:
    double keyExchangeEncTime = 0;
    keyClient(Encoder* encoderObjTemp, u_char* keyExchangeKey);
    keyClient(u_char* keyExchangeKey);
    ~keyClient();
    void run();
    void runKeyGenSimulator();
    bool encodeChunk(Data_t& newChunk);
    bool insertMQFromChunker(Data_t& newChunk);
    bool extractMQFromChunker(Data_t& newChunk);
    bool insertMQToEncoder(Data_t& newChunk);
    bool editJobDoneFlag();
    bool setJobDoneFlag();
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
