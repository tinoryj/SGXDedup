//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYCLIENT_HPP
#define GENERALDEDUPSYSTEM_KEYCLIENT_HPP

#include "ssl.hpp"
#include "_messageQueue.hpp"
#include "configure.hpp"
#include "seriazation.hpp"
#include "chunk.hpp"
#include "cache.hpp"

#include <boost/compute/detail/lru_cache.hpp>

#define KEYMANGER_PUBLIC_KEY_FILE "/key/public.pem"

class keyClient{
private:
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    ssl* _keySecurityChannel;
    int _keyBatchSizeMin,_keyBatchSizeMax;


    RSA* _rsa;
    BIO* _key;
    BN_CTX *_bnCTX;
    BIGNUM* _r,invr;
    BIGNUM* _h;

public:
    keyClient();
    ~keyClient();
    void run();
    string keyExchange(SSL* connection,Chunk champion);
    string elimination(string hash);
    string decoration(string key);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
