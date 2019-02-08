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

#define KEYMANGER_PUBLIC_KEY_FILE "key/serverpub.key"

class keyClient{
private:
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    ssl* _keySecurityChannel;
    int _keyBatchSizeMin,_keyBatchSizeMax;


    RSA* _rsa;
    const BIGNUM *_keyN,*_keyE;
    BIO* _keyfile;
    BN_CTX *_bnCTX;

    EVP_PKEY *_prikey,*_pubkey;

public:
    keyClient();
    ~keyClient();
    void run();
    bool insertMQ(Chunk &newChunk);
    string keyExchange(SSL* connection,Chunk champion);
    string elimination(BIGNUM* r,string hash);
    string decoration(BIGNUM* invr,string key);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
