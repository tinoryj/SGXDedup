//
// Created by a on 11/17/18.
//

#include "keyClient.hpp"
#include "openssl/rsa.h"

#include "chrono"
#include "unistd.h"
#include <bits/stdc++.h>

#include <sys/time.h>

extern Configure config;
extern util::keyCache kCache;

keyClient::keyClient()
{
    _keySecurityChannel = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    _inputMQ.createQueue(CHUNKER_TO_KEYCLIENT_MQ, READ_MESSAGE);
    _outputMQ.createQueue(KEYCLIENT_TO_ENCODER_MQ, WRITE_MESSAGE);
    _keyBatchSizeMin = (int)config.getKeyBatchSizeMin();
    _keyBatchSizeMax = (int)config.getKeyBatchSizeMax();

    _bnCTX = BN_CTX_new();
    _keyfile = BIO_new_file(KEYMANGER_PUBLIC_KEY_FILE, "r");

    if (_keyfile == nullptr) {
        cerr << "Can not open keymanger public key file " << KEYMANGER_PUBLIC_KEY_FILE << endl;
        exit(1);
    }

    _pubkey = PEM_read_bio_PUBKEY(_keyfile, nullptr, nullptr, nullptr);

    if (_pubkey == nullptr) {
        cerr << "keymanger public keyfile damage\n";
        exit(1);
    }
    _rsa = EVP_PKEY_get1_RSA(_pubkey);
    RSA_get0_key(_rsa, &_keyN, &_keyE, nullptr);
    BIO_free_all(_keyfile);
}

keyClient::~keyClient()
{
    delete _keySecurityChannel;

    RSA_free(_rsa);
}

void keyClient::run()
{
    std::pair<int, SSL*> con = _keySecurityChannel->sslConnect();

    while (1) {
        Chunk tmpchunk;
        if (!_inputMQ.pop(tmpchunk)) {
            break;
        };
        if (kCache.existsKeyinCache(tmpchunk.getChunkHash())) {
            tempKey = kCache.getKeyFromCache(tmpchunk.getChunkHash());
            tmpchunk.editEncryptKey(tempKey);
        }
    }
    string newKey = keyExchange(con.second, tmpChunk);
    //write to hash cache
    kCache.insertKeyToCache(tmpChunk.getChunkHash(), newKey);

    tmpChunk.editEncryptKey(newKey);
    this->insertMQ(tmpChunk);
}
//close ssl connection
}

string keyClient::keyExchange(SSL* connection, Chunk currentChunk)
{
    string key;
    _keySecurityChannel->sslWrite(connection, currentChunk.getChunkHash());

    //std::cerr<<"KeyClient : request key\n";

    if (!_keySecurityChannel->sslRead(connection, key)) {
        std::cerr << "KeyClient : key server close\n";
        exit(1);
    }

    return key;
}

bool keyClient::insertMQ(Chunk& newChunk)
{
    _outputMQ.push(newChunk);
    Recipe_t* _recipe = newChunk.getRecipePointer();
    keyRecipe_t* kr = &_recipe->_k;
    kr->_body[newChunk.getID()]._chunkKey = newChunk.getEncryptKey();
}
