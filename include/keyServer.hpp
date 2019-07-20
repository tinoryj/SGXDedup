//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYSERVER_HPP
#define GENERALDEDUPSYSTEM_KEYSERVER_HPP

#include "Socket.hpp"
#include "cache.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "kmClient.hpp"
#include "message.hpp"
#include "messageQueue.hpp"
#include "openssl/bn.h"
#include "ssl.hpp"

#include <string>

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "key/server.key"

class keyServer {
private:
    RSA* _rsa;
    BIO* _key;
    const BIGNUM *_keyN, *_keyD;
    bool _enclave_trusted;

public:
    keyServer();
    ~keyServer();
    void run(Socket socket);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
