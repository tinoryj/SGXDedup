#ifndef GENERALDEDUPSYSTEM_KEYSERVER_HPP
#define GENERALDEDUPSYSTEM_KEYSERVER_HPP

#include "cache.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "kmClient.hpp"
#include "messageQueue.hpp"
#include "openssl/bn.h"
#include "socket.hpp"
#include "ssl.hpp"
#include <bits/stdc++.h>

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "key/server.key"

class keyServer {
private:
    RSA* rsa_;
    BIO* key_;
    const BIGNUM *keyN_, *keyD_;

public:
    keyServer();
    ~keyServer();
    void run(Socket socket);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
