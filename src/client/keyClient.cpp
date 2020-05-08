#include "keyClient.hpp"
#include "openssl/rsa.h"
#include <sys/time.h>

extern Configure config;

void PRINT_BYTE_ARRAY_KEY_CLIENT(
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

keyClient::keyClient(Encoder* encoderobjTemp, u_char* keyExchangeKey)
{
    inputMQ_ = new messageQueue<Data_t>;
    encoderObj_ = encoderobjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keySecurityChannel_ = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    sslConnection_ = keySecurityChannel_->sslConnect().second;
}

keyClient::keyClient(u_char* keyExchangeKey, uint64_t keyGenNumber)
{
    inputMQ_ = new messageQueue<Data_t>;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keyGenNumber_ = keyGenNumber;
#ifdef SGX_KEY_GEN_CTR
    keyExchangeXORBase_ = (u_char*)malloc(CHUNK_HASH_SIZE * keyBatchSize_ * sizeof(u_char));
    int ctrModeCounter = keyBatchSize_ * 2;
    u_char nonce[CRYPTO_BLOCK_SZIE - sizeof(int)];
    memset(nonce, 1, CRYPTO_BLOCK_SZIE - sizeof(int));
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce, ctrModeCounter, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase_);
#endif
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
    inputMQ_->~messageQueue();
    delete inputMQ_;
    cout << "KeyClient : key exchange encryption time = " << keyExchangeEncTime << " s" << endl;
}

void keyClient::runKeyGenSimulator()
{

#ifdef BREAK_DOWN
    double keyGenTime = 0;
    double chunkHashGenerateTime = 0;
    double keyExchangeTime = 0;
    long diff;
    double second;
    struct timeval timestartKeySimulator;
    struct timeval timeendKeySimulator;
#endif
    CryptoPrimitive* cryptoObj = new CryptoPrimitive();
    ssl* keySecurityChannel = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    SSL* sslConnection = keySecurityChannel->sslConnect().second;
    int batchNumber = 0;
    uint64_t currentKeyGenNumber = 0;
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_];
    u_char chunkHash[CHUNK_HASH_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
    while (true) {

        if (currentKeyGenNumber < keyGenNumber_) {
            batchNumber++;
            currentKeyGenNumber++;
            u_char chunkHashTemp[CHUNK_HASH_SIZE];
            u_char chunkTemp[5 * CHUNK_HASH_SIZE];
            memset(chunkTemp, currentKeyGenNumber, 5 * CHUNK_HASH_SIZE);
#ifdef BREAK_DOWN
            gettimeofday(&timestartKeySimulator, NULL);
#endif
            cryptoObj->generateHash(chunkTemp, 5 * CHUNK_HASH_SIZE, chunkHashTemp);
#ifdef BREAK_DOWN
            gettimeofday(&timeendKeySimulator, NULL);
            diff = 1000000 * (timeendKeySimulator.tv_sec - timestartKeySimulator.tv_sec) + timeendKeySimulator.tv_usec - timestartKeySimulator.tv_usec;
            second = diff / 1000000.0;
            keyGenTime += second;
            chunkHashGenerateTime += second;
#endif
            memcpy(chunkHash + batchNumber * CHUNK_HASH_SIZE, chunkHashTemp, CHUNK_HASH_SIZE);
        } else {
            JobDoneFlag = true;
        }

        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            int batchedKeySize = 0;
#ifdef BREAK_DOWN
            gettimeofday(&timestartKeySimulator, NULL);
#endif
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, keySecurityChannel, sslConnection, cryptoObj);
#ifdef BREAK_DOWN
            gettimeofday(&timeendKeySimulator, NULL);
            diff = 1000000 * (timeendKeySimulator.tv_sec - timestartKeySimulator.tv_sec) + timeendKeySimulator.tv_usec - timestartKeySimulator.tv_usec;
            second = diff / 1000000.0;
            keyExchangeTime += second;
            keyGenTime += second;
#endif
            memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
            memset(chunkKey, 0, CHUNK_HASH_SIZE * keyBatchSize_);
            batchNumber = 0;
            if (keyExchangeStatus == false) {
                cerr << "KeyClient : key generate error, thread exit" << endl;
                break;
            }
        }
        if (JobDoneFlag) {
            break;
        }
    }
#ifdef BREAK_DOWN
    cerr << "KeyClient : key generate work time = " << keyGenTime << " s, total key generated is " << currentKeyGenNumber << endl;
    cerr << "KeyClient : key exchange work time = " << keyExchangeTime << " s, chunk hash generate time is " << chunkHashGenerateTime << " s" << endl;
#endif
    return;
}

void keyClient::run()
{

#ifdef BREAK_DOWN
    double keyGenTime = 0;
    double chunkHashGenerateTime = 0;
    double keyExchangeTime = 0;
    long diff;
    double second;
    struct timeval timestartKey;
    struct timeval timeendKey;
#endif
    vector<Data_t> batchList;
    int batchNumber = 0;
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_];
    u_char chunkHash[CHUNK_HASH_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
    while (true) {

        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            cerr << "KeyClient : Chunker jobs done, queue is empty" << endl;
            JobDoneFlag = true;
        }
        if (extractMQFromChunker(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                insertMQToEncoder(tempChunk);
                continue;
            }
            batchList.push_back(tempChunk);
#ifdef BREAK_DOWN
            gettimeofday(&timestartKey, NULL);
#endif
            cryptoObj_->generateHash(tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize, tempChunk.chunk.chunkHash);
#ifdef BREAK_DOWN
            gettimeofday(&timeendKey, NULL);
            diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
            second = diff / 1000000.0;
            keyGenTime += second;
            chunkHashGenerateTime += second;
#endif
            memcpy(chunkHash + batchNumber * CHUNK_HASH_SIZE, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            batchNumber++;
        }
        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            int batchedKeySize = 0;
#ifdef BREAK_DOWN
            gettimeofday(&timestartKey, NULL);
#endif
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize);
#ifdef BREAK_DOWN
            gettimeofday(&timeendKey, NULL);
            diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
            second = diff / 1000000.0;
            keyGenTime += second;
            keyExchangeTime += second;
#endif
            if (!keyExchangeStatus) {
                cerr << "KeyClient : error get key for " << setbase(10) << batchNumber << " chunks" << endl;
                return;
            } else {
                for (int i = 0; i < batchNumber; i++) {
                    memcpy(batchList[i].chunk.encryptKey, chunkKey + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
                    insertMQToEncoder(batchList[i]);
                }
                batchList.clear();
                memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
                memset(chunkKey, 0, CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_);
                batchNumber = 0;
            }
        }
        if (JobDoneFlag) {
            if (!encoderObj_->editJobDoneFlag()) {
                cerr << "KeyClient : error to set job done flag for encoder" << endl;
            } else {
                cerr << "KeyClient : key exchange thread job done, set job done flag for encoder done, exit now" << endl;
            }
            break;
        }
    }
#ifdef BREAK_DOWN
    cout << "KeyClient : keyGen total work time = " << keyGenTime << " s" << endl;
    cout << "KeyClient : chunk hash generate time = " << chunkHashGenerateTime << " s" << endl;
    cout << "KeyClient : key exchange encrypt work time = " << keyExchangeEncTime << " s" << endl;
    cout << "KeyClient : key exchange work time = " << keyExchangeTime << " s" << endl;
#endif
    return;
}

#ifdef SGX_KEY_GEN
bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    // #ifdef BREAK_DOWN
    //     struct timeval timestartKey_enc;
    //     struct timeval timeendKey_enc;
    //     gettimeofday(&timestartKey_enc, NULL);
    // #endif
    cryptoObj_->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    // #ifdef BREAK_DOWN
    //     gettimeofday(&timeendKey_enc, NULL);
    //     long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    //     double second = diff / 1000000.0;
    //     keyExchangeEncTime += second;
    // #endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "keyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "keyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
        return false;
    }
    batchkeyNumber = recvSize / CHUNK_ENCRYPT_KEY_SIZE;
    if (batchkeyNumber == batchNumber) {
        // #ifdef BREAK_DOWN
        //         gettimeofday(&timestartKey_enc, NULL);
        // #endif
        cryptoObj_->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
        // #ifdef BREAK_DOWN
        //         gettimeofday(&timeendKey_enc, NULL);
        //         diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
        //         second = diff / 1000000.0;
        //         keyExchangeEncTime += second;
        // #endif
        return true;
    } else {
        return false;
    }
}

#ifdef SGX_KEY_GEN_CTR
bool keyClient::keyExchangeXOR(u_char* result, u_char* input, int batchNumber)
{
    for (int i = 0; i < batchNumber * CHUNK_HASH_SIZE; i++) {
        result[i] = input[i] ^ keyExchangeXORBase_[i];
    }
    return true;
}
#endif

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
// #ifdef BREAK_DOWN
//     struct timeval timestartKey_enc;
//     struct timeval timeendKey_enc;
//     gettimeofday(&timestartKey_enc, NULL);
// #endif
#ifdef SGX_KEY_GEN_CTR
    keyExchangeXOR(sendHash, batchHashList, batchNumber);
#else
    cryptoObj->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
#endif

    // #ifdef BREAK_DOWN
    //     gettimeofday(&timeendKey_enc, NULL);
    //     long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    //     double second = diff / 1000000.0;
    //     keyExchangeEncTime += second;
    // #endif

    if (!securityChannel->send(sslConnection, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "keyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!securityChannel->recv(sslConnection, (char*)recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "keyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
        return false;
    }
    batchkeyNumber = recvSize / CHUNK_ENCRYPT_KEY_SIZE;
    if (batchkeyNumber == batchNumber) {
        // #ifdef BREAK_DOWN
        //         gettimeofday(&timestartKey_enc, NULL);
        // #endif
#ifdef SGX_KEY_GEN_CTR
        keyExchangeXOR(batchKeyList, recvBuffer, batchkeyNumber);
#else
        cryptoObj->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
#endif
        // #ifdef BREAK_DOWN
        //         gettimeofday(&timeendKey_enc, NULL);
        //         diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
        //         second = diff / 1000000.0;
        //         keyExchangeEncTime += second;
        // #endif
        return true;
    } else {
        return false;
    }
}
#endif

#ifdef NO_OPRF
bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    if (!keySecurityChannel_->send(sslConnection_, (char*)batchHashList, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "keyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "keyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
        return false;
    }
    batchkeyNumber = recvSize / CHUNK_ENCRYPT_KEY_SIZE;
    if (batchkeyNumber == batchNumber) {
        memcpy(batchKeyList, recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE);
        return true;
    } else {
        return false;
    }
}

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection)
{
    if (!securityChannel->send(sslConnection, (char*)batchHashList, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "keyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!securityChannel->recv(sslConnection, (char*)recvBuffer, recvSize)) {
        cerr << "keyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "keyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
        return false;
    }
    batchkeyNumber = recvSize / CHUNK_ENCRYPT_KEY_SIZE;
    if (batchkeyNumber == batchNumber) {
        memcpy(batchKeyList, recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE);
        return true;
    } else {
        return false;
    }
}
#endif

bool keyClient::insertMQFromChunker(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool keyClient::extractMQFromChunker(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool keyClient::insertMQToEncoder(Data_t& newChunk)
{
    return encoderObj_->insertMQFromKeyClient(newChunk);
}

bool keyClient::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}
