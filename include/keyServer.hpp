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
    uint64_t sessionKeyRegressionMaxNumber_, sessionKeyRegressionCurrentNumber_;
    bool raRequestFlag_, raSetupFlag_, sessionKeyUpdateFlag_;
    std::mutex mutexSessionKeyUpdate;
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    bool offlineGenerateFlag_ = false;
#endif
    ssl* keySecurityChannel_;
#if KEY_GEN_EPOLL_MODE == 1
    messageQueue<int>* requestMQ_; // only transmit fd
    messageQueue<int>* responseMQ_; // only transmit fd
    vector<uint64_t> perThreadKeyGenerateCount_;
    unordered_map<int, KeyServerEpollMessage_t> epollSession_;
    unordered_map<int, SSL*> sslConnectionList_;
    int epfd_;
#endif
public:
    keyServer(ssl* keySecurityChannelTemp);
    ~keyServer();
    bool runRemoteAttestationInit();
    void runRAwithSPRequest();
    void runSessionKeyUpdate();
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    void runCTRModeMaskGenerate();
#endif
#if KEY_GEN_EPOLL_MODE == 1
    void runRecvThread();
    void runSendThread();
    void runKeyGenerateRequestThread(int threadID);
#else
    void runKeyGenerateThread(SSL* connection);
#endif
    bool initEnclaveViaRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection);
    bool getRASetupFlag();
};

#endif //SGXDEDUP_KEYSERVER_HPP
