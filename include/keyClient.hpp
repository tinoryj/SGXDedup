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
    int keyGenNumber_;
#ifdef SGX_KEY_GEN_CTR
    u_char* keyExchangeXORBase_;
#endif

public:
    double keyExchangeEncTime = 0;
    keyClient(Encoder* encoderObjTemp, u_char* keyExchangeKey);
    keyClient(u_char* keyExchangeKey, uint64_t keyGenNumber);
    ~keyClient();
    void run();
    void runKeyGenSimulator();
    bool encodeChunk(Data_t& newChunk);
    bool insertMQFromChunker(Data_t& newChunk);
    bool extractMQFromChunker(Data_t& newChunk);
    bool insertMQToEncoder(Data_t& newChunk);
    bool editJobDoneFlag();
    bool setJobDoneFlag();
    bool keyExchangeXOR(u_char* result, u_char* input, int batchNumber);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
