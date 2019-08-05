#include "keyClient.hpp"
#include "openssl/rsa.h"

extern Configure config;
extern KeyCache kCache;

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
        cerr << "KeyClient : keyServer enclave not trusted, client exit now" << endl;
        exit(0);
    }
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
}
void PRINT_BYTE_ARRAY(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}
void keyClient::run()
{
    while (true) {
        string chunkKey, chunkHash;
        chunkHash.resize(CHUNK_HASH_SIZE);
        chunkKey.resize(CHUNK_ENCRYPT_KEY_SIZE);
        Data_t tempchunk;
        if (inputMQ_.done_ && !extractMQFromChunker(tempchunk)) {
            break;
        };
        if (tempchunk.dataType == DATA_TYPE_RECIPE) {
            continue;
        } else if (tempchunk.dataType == DATA_TYPE_CHUNK) {
            cerr << "KeyClient : new chunk ID = " << tempchunk.chunk.ID << endl;
            PRINT_BYTE_ARRAY(stderr, tempchunk.chunk.chunkHash, CHUNK_HASH_SIZE);
        } else {
            cerr << "KeyClient : extract data type = " << tempchunk.dataType << endl;
        }
        memcpy(&chunkKey[0], tempchunk.chunk.chunkHash, CHUNK_HASH_SIZE);

        if (kCache.existsKeyinCache(chunkHash)) {
            exit(0);
            //cerr << "KeyClient : chunkHash hit kCache" << endl;
            chunkKey = kCache.getKeyFromCache(chunkHash);
            memcpy(tempchunk.chunk.encryptKey, &chunkKey[0], CHUNK_ENCRYPT_KEY_SIZE);
        } else {
            //cerr << "KeyClient : chunkHash not hit kCache, start key exchange" << endl;
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
    u_char chunkHash[CHUNK_HASH_SIZE];
    memcpy(chunkHash, newChunk.chunkHash, CHUNK_HASH_SIZE);
    u_char sendHash[CHUNK_HASH_SIZE];
    cryptoObj_->keyExchangeEncrypt(chunkHash, CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    if (!socket_.Send(sendHash, CHUNK_HASH_SIZE)) {
        cerr << "keyClient: send socket error" << endl;
        exit(0);
    }
    u_char recvBuffer[NETWORK_RESPOND_BUFFER_MAX_SIZE];
    int recvSize;
    if (!socket_.Recv(recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        exit(0);
    } else if (recvSize != CHUNK_ENCRYPT_KEY_SIZE) {
        cerr << "keyClient: recv socket error - recv size not equal to CHUNK_ENCRYPT_KEY_SIZE" << endl;
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