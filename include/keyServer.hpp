#ifndef SGXDEDUP_KEYSERVER_HPP
#define SGXDEDUP_KEYSERVER_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include "kmClient.hpp"
#include "messageQueue.hpp"
#include "openssl/bn.h"
#include "ssl.hpp"
#include <bits/stdc++.h>
#if KEY_GEN_EPOLL_MODE == 1
#include <sys/epoll.h>
#endif
#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "key/sslKeys/server-key.pem"

class keyServer {
private:
    RSA* rsa_;
    BIO* key_;
    const BIGNUM *keyN_, *keyD_;
    kmClient* client;
    std::mutex multiThreadMutex_;
    std::mutex multiThreadCountMutex_;
    std::mutex clientThreadNumberCountMutex_;
    uint64_t keyGenerateCount_;
    uint64_t clientThreadCount_;
    uint64_t keyGenLimitPerSessionKey_;
    uint64_t sessionKeyUpdateCount_;
    bool raRequestFlag;
    bool raSetupFlag;
    ssl* keySecurityChannel_;
#if KEY_GEN_EPOLL_MODE == 1
    messageQueue<int>* requestMQ_; // only transmit fd
    messageQueue<int>* responseMQ_; // only transmit fd
    vector<uint64_t> perThreadKeyGenerateCount_;
    unordered_map<int, KeyServerEpollMessage_t> epollSession_;
    unordered_map<int, SSL*> sslConnectionList_;
    int epfd_;
#endif
#if KEY_GEN_SGX_CTR == 1
    typedef struct {
        int clientID;
        uint32_t keyGenerateCounter = 0;
        uint32_t currentKeyGenerateCounter = 0;
        u_char nonce[16 - sizeof(uint32_t)];
        int nonceLen = 16 - sizeof(uint32_t);
        bool offLineGenerateFlag = false;
    } MaskInfo_t;
    unordered_map<int, MaskInfo_t> clientList_; //clientID - MaskInfo_t pair
#endif
public:
    keyServer(ssl* keySecurityChannelTemp);
    ~keyServer();
    void runRA();
    void runRAwithSPRequest();
    void runSessionKeyUpdate();
#if KEY_GEN_SGX_CTR == 1
    void runCTRModeMaskGenerate();
#endif
#if KEY_GEN_EPOLL_MODE == 1
    void runRecvThread();
    void runSendThread();
    void runKeyGenerateRequestThread(int threadID);
#else
    void runKeyGenerateThread(SSL* connection);
#endif
    bool doRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection);
    bool getRASetupFlag();
};

#endif //SGXDEDUP_KEYSERVER_HPP
