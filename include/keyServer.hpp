#ifndef GENERALDEDUPSYSTEM_KEYSERVER_HPP
#define GENERALDEDUPSYSTEM_KEYSERVER_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include "kmClient.hpp"
#include "messageQueue.hpp"
#include "openssl/bn.h"
#include "socket.hpp"
#include <bits/stdc++.h>

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "key/server.key"

class keyServer {
private:
    RSA* rsa_;
    BIO* key_;
    const BIGNUM *keyN_, *keyD_;
    kmClient* client;
    std::mutex multiThreadMutex_;
    std::mutex multiThreadCountMutex_;
    std::mutex clientThreadNumberCountMutex_;
    uint64_t keyGenerateCount;
    uint64_t clientThreadCount;
    uint64_t keyGenLimitPerSessionKey_;
    bool raRequestFlag;

public:
    keyServer();
    ~keyServer();
    void run(Socket socket);
    void runRA();
    void runRAwithSPRequest();
    bool doRemoteAttestation(Socket socket);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
