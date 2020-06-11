#ifndef GENERALDEDUPSYSTEM_KEYSERVER_HPP
#define GENERALDEDUPSYSTEM_KEYSERVER_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include "kmClient.hpp"
#include "messageQueue.hpp"
#include "openssl/bn.h"
#include "ssl.hpp"
#include <bits/stdc++.h>

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
    ssl* keySecurityChannel_;
#if SGX_KEY_GEN_CTR == 1
    typedef struct {
        int clientID;
        uint32_t keyGenerateCounter = 0;
        u_char nonce[16 - sizeof(uint32_t)];
    } maskInfo;
    vector<maskInfo> clientList;
#endif

public:
    keyServer(ssl* keySecurityChannelTemp);
    ~keyServer();
    void run(SSL* connection);
    void runRA();
    void runRAwithSPRequest();
    void runSessionKeyUpdate();
#if SGX_KEY_GEN_CTR == 1
    void runCTRModeMaskGenerate();
#endif
    bool doRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
