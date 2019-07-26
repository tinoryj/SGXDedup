#include "keyClient.hpp"
#include "openssl/rsa.h"

extern Configure config;
extern keyCache kCache;

keyClient::keyClient(Encoder* encoderObjTemp)
{
    encoderObj_ = encoderObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSizeMin_ = (int)config.getKeyBatchSizeMin();
    keyBatchSizeMax_ = (int)config.getKeyBatchSizeMax();
    socket_.init(CLIENT_TCP, config.getKeyServerIP(), config.getKeyServerPort());
    kmServer server(socket_);
    powSession* session = server.authkm();
    if (session != nullptr) {
        trustdKM_ = true;
        memcpy(keyExchangeKey_, session->smk, 16);
        delete session;
    } else {
        trustdKM_ = false;
        delete session;
    }
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}

void keyClient::run()
{
    while (true) {
        string chunkKey, chunkHash;
        chunkHash.resize(CHUNK_HASH_SIZE);
        Data_t tempchunk;
        if (inputMQ_.done_ && !extractMQFromChunker(tempchunk)) {
            break;
        };
        if (tempchunk.dataType == DATA_TYPE_RECIPE) {
            continue;
        }
        memcpy(&chunkKey[0], tempchunk.chunk.chunkHash, CHUNK_HASH_SIZE);
        if (kCache.existsKeyinCache(chunkHash)) {
            chunkKey = kCache.getKeyFromCache(chunkHash);
            memcpy(tempchunk.chunk.encryptKey, &chunkKey[0], CHUNK_ENCRYPT_KEY_SIZE);
        } else {
            chunkKey = keyExchange(tempchunk.chunk);
            kCache.insertKeyToCache(chunkHash, chunkKey);
            memcpy(tempchunk.chunk.encryptKey, &chunkKey[0], CHUNK_ENCRYPT_KEY_SIZE);
        }
        insertMQtoEncoder(tempchunk);
    }
    pthread_exit(NULL);
    //close ssl connection
}

string keyClient::keyExchange(Chunk_t newChunk)
{
    while (!trustdKM_)
        ;
    u_char chunkHash[CHUNK_HASH_SIZE];
    memcpy(chunkHash, newChunk.chunkHash, CHUNK_HASH_SIZE);
    u_char sendHash[CHUNK_HASH_SIZE];
    cryptoObj_->keyExchangeEncrypt(chunkHash, CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    if (!socket_.Send(sendHash, CHUNK_HASH_SIZE)) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    u_char recvBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;
    if (!socket_.Recv(recvBuffer, recvSize) || recvSize != CHUNK_ENCRYPT_KEY_SIZE) {
        cerr << "keyClient: socket error" << endl;
        exit(0);
    }
    u_char key[CHUNK_ENCRYPT_KEY_SIZE];
    cryptoObj_->keyExchangeDecrypt(recvBuffer, recvSize, keyExchangeKey_, keyExchangeKey_, key);
    u_char returnValue_char[CHUNK_ENCRYPT_KEY_SIZE];
    memcpy(returnValue_char, key, CHUNK_ENCRYPT_KEY_SIZE);
    string returnValue;
    returnValue.resize(CHUNK_ENCRYPT_KEY_SIZE);
    memcpy(&returnValue[0], returnValue_char, CHUNK_ENCRYPT_KEY_SIZE);
    return returnValue;
}

bool keyClient::insertMQFromChunker(Data_t& newChunk)
{
    return inputMQ_.push(newChunk);
}

bool keyClient::extractMQFromChunker(Data_t& newChunk)
{
    return inputMQ_.pop(newChunk);
}

bool keyClient::insertMQtoEncoder(Data_t& newChunk)
{
    return encoderObj_->insertMQFromKeyClient(newChunk);
}

bool keyClient::editJobDoneFlag()
{
    inputMQ_.done_ = true;
    if (inputMQ_.done_) {
        return true;
    } else {
        return false;
    }
}