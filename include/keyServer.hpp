//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYSERVER_HPP
#define GENERALDEDUPSYSTEM_KEYSERVER_HPP

#include "_keyManager.hpp"
#include "chunk.hpp"
#include "ssl.hpp"
#include "configure.hpp"
#include "cache.hpp"
#include "_messageQueue.hpp"
#include "message.hpp"
#include "kmClient.hpp"
#include "Socket.hpp"
#include "openssl/bn.h"

#include <string>

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "key/server.key"



class keyServer:public _keyManager{
private:
    RSA* _rsa;
    BIO* _key;
    const BIGNUM *_keyN,*_keyD;
    bool _enclave_trusted;


public:
    keyServer();
    ~keyServer();
    void run(Socket socket);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
