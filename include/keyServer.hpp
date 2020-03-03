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

public:
    keyServer(ssl* keySecurityChannelTemp);
    ~keyServer();
    void run(SSL* connection);
    void runRA();
    void runRAwithSPRequest();
    bool doRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
