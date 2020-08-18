#ifndef SGXDEDUP_KEYCLIENT_HPP
#define SGXDEDUP_KEYCLIENT_HPP

#include "configure.hpp"
#if ENCODER_MODULE_ENABLED == 1
#include "encoder.hpp"
#else
#include "enclaveSession.hpp"
#include "powClient.hpp"
#endif
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include "ssl.hpp"

class KeyClient {
private:
    messageQueue<Data_t>* inputMQ_;
#if ENCODER_MODULE_ENABLED == 1
    Encoder* encoderObj_;
#else
    powClient* powObj_;
#endif
    CryptoPrimitive* cryptoObj_;
    int keyBatchSize_;
    ssl* keySecurityChannel_;
    SSL* sslConnection_;
    u_char keyExchangeKey_[KEY_SERVER_SESSION_KEY_SIZE];
    uint64_t keyGenNumber_;
    int clientID_;
    std::mutex mutexkeyGenerateSimulatorEncTime_;
    std::mutex mutexkeyGenerateSimulatorStart_;
    vector<timeval> keyGenSimulatorStartTimeCounter_;
    vector<timeval> keyGenSimulatorEndTimeCounter_;
    int totalSimulatorThreadNumber_;
    int currentInitThreadNumber_;
    int batchNumber_;

public:
    double keyExchangeEncTime = 0;

#if ENCODER_MODULE_ENABLED == 1
    KeyClient(Encoder* encoderObjTemp, u_char* keyExchangeKey);
#else
    KeyClient(powClient* powObjTemp, u_char* keyExchangeKey);
#endif
    KeyClient(u_char* keyExchangeKey, int threadNumber, uint64_t keyGenNumber, int batchSize);
    ~KeyClient();
    bool outputKeyGenSimulatorRunningTime();
    void run();
    void runKeyGenSimulator(int clientID);
    bool insertMQ(Data_t& newChunk);
    bool extractMQ(Data_t& newChunk);
    bool editJobDoneFlag();
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj);
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    double keyExchangeMaskGenerateTime = 0;
    u_char nonce_[CRYPTO_BLOCK_SZIE - sizeof(uint32_t)];
    uint32_t counter_ = 0;
    bool initClientCTRInfo();
    bool saveClientCTRInfo();
    bool keyExchangeXOR(u_char* result, u_char* input, u_char* xorBase, int batchNumber);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, NetworkHeadStruct_t netHead);
    bool keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj, u_char* nonce, uint32_t counter, NetworkHeadStruct_t netHead);
#endif
};

#endif //SGXDEDUP_KEYCLIENT_HPP
