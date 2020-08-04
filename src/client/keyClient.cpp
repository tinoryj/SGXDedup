#include "keyClient.hpp"
#include "openssl/rsa.h"
#include <sys/time.h>

extern Configure config;

struct timeval timestartKey;
struct timeval timeendKey;
struct timeval timestartKeySimulator;
struct timeval timeendKeySimulator;

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
#if ENCODER_MODULE_ENABLED == 1

KeyClient::KeyClient(Encoder* encoderobjTemp, u_char* keyExchangeKey)
{
    inputMQ_ = new messageQueue<Data_t>;
    encoderObj_ = encoderobjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keySecurityChannel_ = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    sslConnection_ = keySecurityChannel_->sslConnect().second;
    clientID_ = config.getClientID();
}

#else

KeyClient::KeyClient(powClient* powObjTemp, u_char* keyExchangeKey)
{
    inputMQ_ = new messageQueue<Data_t>;
    powObj_ = powObjTemp;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keySecurityChannel_ = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    sslConnection_ = keySecurityChannel_->sslConnect().second;
    clientID_ = config.getClientID();
}
#endif

KeyClient::KeyClient(u_char* keyExchangeKey, uint64_t keyGenNumber)
{
    inputMQ_ = new messageQueue<Data_t>;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keyGenNumber_ = keyGenNumber;
    clientID_ = config.getClientID();
}

KeyClient::~KeyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
#if QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_SPSC_QUEUE || QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_QUEUE
    inputMQ_->~messageQueue();
    delete inputMQ_;
#endif
}

void KeyClient::runKeyGenSimulator(int clientID)
{

#if SYSTEM_BREAK_DOWN == 1
    double keyGenTime = 0;
    double chunkHashGenerateTime = 0;
    double keyExchangeTime = 0;
    long diff;
    double second;
#endif
    CryptoPrimitive* cryptoObj = new CryptoPrimitive();
    ssl* keySecurityChannel = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), CLIENTSIDE);
    SSL* sslConnection = keySecurityChannel->sslConnect().second;
    int batchNumber = 0;
    uint64_t currentKeyGenNumber = 0;
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_];
    u_char chunkHash[CHUNK_HASH_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
#if KEY_GEN_SGX_CTR == 1
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKeySimulator, NULL);
#endif
    u_char nonce[CRYPTO_BLOCK_SZIE - sizeof(uint32_t)];
    uint32_t counter = 0;
    //read old counter
    string keyGenFileName = ".keyGenStore" + to_string(clientID);
    ifstream keyGenStoreIn;
    keyGenStoreIn.open(keyGenFileName, std::ifstream::in | std::ifstream::binary);
    if (keyGenStoreIn.is_open()) {
        keyGenStoreIn.seekg(0, ios_base::end);
        int counterFileSize = keyGenStoreIn.tellg();
        keyGenStoreIn.seekg(0, ios_base::beg);
        if (counterFileSize != 16) {
            cerr << "KeyClient : stored old counter file size error" << endl;
        } else {
            char readBuffer[16];
            keyGenStoreIn.read(readBuffer, 16);
            keyGenStoreIn.close();
            if (keyGenStoreIn.gcount() != 16) {
                cerr << "KeyClient : read old counter file size error" << endl;
            } else {
                memcpy(nonce, readBuffer, 12);
                memcpy(&counter, readBuffer + 12, sizeof(uint32_t));
#if SYSTEM_DEBUG_FLAG == 1
                cerr << "KeyClient : Read old counter file : " << keyGenFileName << " success, the original counter = " << counter << ", nonce = " << endl;
                PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, nonce, 12);
#endif
            }
        }
    } else {
    nonceUsedRetry:
        srand(time(NULL));
        for (int i = 0; i < 12 / sizeof(int); i++) {
            int randomNumber = rand();
            memcpy(nonce + i * sizeof(int), &randomNumber, sizeof(int));
        }
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : Can not open old counter file : \"" << keyGenFileName << "\", Directly reset counter to 0, generate nonce = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, nonce, 12);
#endif
    }
    // done
    NetworkHeadStruct_t initHead, responseHead;
    initHead.clientID = clientID;
    initHead.dataSize = 48;
    initHead.messageType = KEY_GEN_UPLOAD_CLIENT_INFO;
    char initInfoBuffer[sizeof(NetworkHeadStruct_t) + initHead.dataSize]; // clientID & nonce & counter
    char responseBuffer[sizeof(NetworkHeadStruct_t)];
    memcpy(initInfoBuffer, &initHead, sizeof(NetworkHeadStruct_t));
    u_char tempCipherBuffer[16], tempPlaintBuffer[16];
    memcpy(tempPlaintBuffer, &counter, sizeof(uint32_t));
    memcpy(tempPlaintBuffer + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
    cryptoObj->keyExchangeEncrypt(tempPlaintBuffer, 16, keyExchangeKey_, keyExchangeKey_, tempCipherBuffer);
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t), tempCipherBuffer, 16);
    cryptoObj->sha256Hmac(tempCipherBuffer, 16, (u_char*)initInfoBuffer + sizeof(NetworkHeadStruct_t) + 16, keyExchangeKey_, 32);
    if (!keySecurityChannel->send(sslConnection, initInfoBuffer, sizeof(NetworkHeadStruct_t) + initHead.dataSize)) {
        cerr << "KeyClient: send init information error" << endl;
        return;
    } else {
        int recvSize;
        if (!keySecurityChannel->recv(sslConnection, responseBuffer, recvSize)) {
            cerr << "KeyClient: recv init information status error" << endl;
            return;
        } else {
            memcpy(&responseHead, responseBuffer, sizeof(NetworkHeadStruct_t));
            if (responseHead.messageType == CLIENT_COUNTER_REST) {
                cerr << "KeyClient : key server counter error, reset client counter to 0" << endl;
                counter = 0;
            } else if (responseHead.messageType == NONCE_HAS_USED) {
                cerr << "KeyClient: nonce has used, goto retry" << endl;
                goto nonceUsedRetry;
            } else if (responseHead.messageType == ERROR_RESEND) {
                cerr << "KeyClient: hmac error, goto retry" << endl;
                goto nonceUsedRetry;
            }
        }
        cerr << "KeyClient : init information success, start key generate" << endl;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKeySimulator, NULL);
    diff = 1000000 * (timeendKeySimulator.tv_sec - timestartKeySimulator.tv_sec) + timeendKeySimulator.tv_usec - timestartKeySimulator.tv_usec;
    second = diff / 1000000.0;
    cout << "KeyClient : init ctr mode time = " << second << endl;
#endif
#endif
    NetworkHeadStruct_t dataHead;
    dataHead.clientID = clientID;
    dataHead.messageType = KEY_GEN_UPLOAD_CHUNK_HASH;
    u_char chunkHashTemp[CHUNK_HASH_SIZE];
    u_char chunkTemp[5 * CHUNK_HASH_SIZE];
    while (true) {

        if (currentKeyGenNumber < keyGenNumber_) {
            batchNumber++;
            currentKeyGenNumber++;
            memset(chunkTemp, currentKeyGenNumber, 5 * CHUNK_HASH_SIZE);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartKeySimulator, NULL);
#endif
            cryptoObj->generateHash(chunkTemp, 5 * CHUNK_HASH_SIZE, chunkHashTemp);
#if SYSTEM_BREAK_DOWN == 1
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
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartKeySimulator, NULL);
#endif
#if KEY_GEN_SGX_CTR == 1
            dataHead.dataSize = batchNumber * CHUNK_HASH_SIZE;
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, keySecurityChannel, sslConnection, cryptoObj, nonce, counter, dataHead);
            counter += batchNumber * 4;
#else
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, keySecurityChannel, sslConnection, cryptoObj);
#endif
#if SYSTEM_BREAK_DOWN == 1
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
#if KEY_GEN_SGX_CTR == 1
    ofstream counterOut;
    counterOut.open(keyGenFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << keyGenFileName << endl;
    } else {
        char writeBuffer[sizeof(uint32_t)];
        memcpy(writeBuffer, &counter, sizeof(uint32_t));
        counterOut.write(writeBuffer, sizeof(uint32_t));
        counterOut.close();
        cerr << "KeyClient : Stored current counter file : " << keyGenFileName << ", counter = " << counter << endl;
    }
#endif
#if SYSTEM_BREAK_DOWN == 1
    cout << "KeyClient : client ID = " << clientID << endl;
    cout << "KeyClient : key generate work time = " << keyGenTime << " s, total key generated is " << currentKeyGenNumber << endl;
    cout << "KeyClient : key exchange work time = " << keyExchangeTime << " s, chunk hash generate time is " << chunkHashGenerateTime << " s" << endl;
#endif
    // delete cryptoObj;
    // delete keySecurityChannel;
    return;
}

void KeyClient::run()
{

#if SYSTEM_BREAK_DOWN == 1
    double keyGenTime = 0;
    double chunkContentEncryptionTime = 0;
#if FINGERPRINTER_MODULE_ENABLE == 0
    double generatePlainChunkHashTime = 0;
#endif // FINGERPRINTER_MODULE_ENABLE
    long diff;
    double second;
#endif // SYSTEM_BREAK_DOWN
    vector<Data_t> batchList;
    int batchNumber = 0;
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_];
    u_char chunkHash[CHUNK_HASH_SIZE * keyBatchSize_];
    bool JobDoneFlag = false;
#if KEY_GEN_SGX_CTR == 1
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey, NULL);
#endif // SYSTEM_BREAK_DOWN
    u_char nonce[CRYPTO_BLOCK_SZIE - sizeof(uint32_t)];
    uint32_t counter = 0;
    //read old counter
    string keyGenFileName = ".keyGenStore";
    ifstream keyGenStoreIn;
    keyGenStoreIn.open(keyGenFileName, std::ifstream::in | std::ifstream::binary);
    if (keyGenStoreIn.is_open()) {
        keyGenStoreIn.seekg(0, ios_base::end);
        int counterFileSize = keyGenStoreIn.tellg();
        keyGenStoreIn.seekg(0, ios_base::beg);
        if (counterFileSize != 16) {
            cerr << "KeyClient : stored old counter file size error" << endl;
        } else {
            char readBuffer[16];
            keyGenStoreIn.read(readBuffer, 16);
            keyGenStoreIn.close();
            if (keyGenStoreIn.gcount() != 16) {
                cerr << "KeyClient : read old counter file size error" << endl;
            } else {
                memcpy(nonce, readBuffer, 12);
                memcpy(&counter, readBuffer + 12, sizeof(uint32_t));
#if SYSTEM_DEBUG_FLAG == 1
                cerr << "KeyClient : Read old counter file : " << keyGenFileName << " success, the original counter = " << counter << ", nonce = " << endl;
                PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, nonce, 12);
#endif
            }
        }
    } else {
    nonceUsedRetry:
        srand(time(NULL));
        for (int i = 0; i < 12 / sizeof(int); i++) {
            int randomNumber = rand();
            memcpy(nonce + i * sizeof(int), &randomNumber, sizeof(int));
        }
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : Can not open old counter file : \"" << keyGenFileName << "\", Directly reset counter to 0, generate nonce = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, nonce, 12);
#endif
    }
    // done
    NetworkHeadStruct_t initHead, responseHead;
    initHead.clientID = clientID_;
    initHead.dataSize = 48;
    initHead.messageType = KEY_GEN_UPLOAD_CLIENT_INFO;
    char initInfoBuffer[sizeof(NetworkHeadStruct_t) + initHead.dataSize]; // clientID & nonce & counter
    char responseBuffer[sizeof(NetworkHeadStruct_t)];
    memcpy(initInfoBuffer, &initHead, sizeof(NetworkHeadStruct_t));
    u_char tempCipherBuffer[16], tempPlaintBuffer[16];
    memcpy(tempPlaintBuffer, &counter, sizeof(uint32_t));
    memcpy(tempPlaintBuffer + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
    cryptoObj_->keyExchangeEncrypt(tempPlaintBuffer, 16, keyExchangeKey_, keyExchangeKey_, tempCipherBuffer);
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t), tempCipherBuffer, 16);
    cryptoObj_->sha256Hmac(tempCipherBuffer, 16, (u_char*)initInfoBuffer + sizeof(NetworkHeadStruct_t) + 16, keyExchangeKey_, 32);
    if (!keySecurityChannel_->send(sslConnection_, initInfoBuffer, sizeof(NetworkHeadStruct_t) + initHead.dataSize)) {
        cerr << "KeyClient: send init information error" << endl;
        return;
    } else {
        int recvSize;
        if (!keySecurityChannel_->recv(sslConnection_, responseBuffer, recvSize)) {
            cerr << "KeyClient: recv init information status error" << endl;
            return;
        } else {
            memcpy(&responseHead, responseBuffer, sizeof(NetworkHeadStruct_t));
            cerr << "KeyClient : recv key server response" << endl;
            if (responseHead.messageType == CLIENT_COUNTER_REST) {
                cerr << "KeyClient : key server counter error, reset client counter to 0" << endl;
                counter = 0;
            } else if (responseHead.messageType == NONCE_HAS_USED) {
                cerr << "KeyClient: nonce has used, goto retry" << endl;
                goto nonceUsedRetry;
            } else if (responseHead.messageType == ERROR_RESEND) {
                cerr << "KeyClient: hmac error, goto retry" << endl;
                goto nonceUsedRetry;
            } else if (responseHead.messageType == SUCCESS) {
                cerr << "KeyClient : init information success, start key generate" << endl;
            }
        }
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey, NULL);
    diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
    second = diff / 1000000.0;
    cout << "KeyClient : init ctr mode time = " << second << endl;
#endif // SYSTEM_BREAK_DOWN
#endif // KEY_GEN_SGX_CTR
    NetworkHeadStruct_t dataHead;
    dataHead.clientID = clientID_;
    dataHead.messageType = KEY_GEN_UPLOAD_CHUNK_HASH;
    while (true) {
        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            JobDoneFlag = true;
        }
        if (extractMQ(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
#if ENCODER_MODULE_ENABLED == 1
                encoderObj_->insertMQ(tempChunk);
#else
                powObj_->insertMQ(tempChunk);
#endif
                continue;
            }
            batchList.push_back(tempChunk);
#if FINGERPRINTER_MODULE_ENABLE == 0
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartKey, NULL);
#endif
            bool generatePlainChunkHashStatus = cryptoObj_->generateHash(tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize, tempChunk.chunk.chunkHash);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendKey, NULL);
            diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
            second = diff / 1000000.0;
            generatePlainChunkHashTime += second;
#endif
#endif
            memcpy(chunkHash + batchNumber * CHUNK_HASH_SIZE, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
            batchNumber++;
        }
        if (batchNumber == keyBatchSize_ || JobDoneFlag) {
            int batchedKeySize = 0;
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartKey, NULL);
#endif
#if KEY_GEN_SGX_CTR == 1
            dataHead.dataSize = batchNumber * CHUNK_HASH_SIZE;
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, nonce, counter, dataHead);
            counter += batchNumber * 4;
#else
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize);
#endif

#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendKey, NULL);
            diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
            second = diff / 1000000.0;
            keyGenTime += second;
#endif
            if (!keyExchangeStatus) {
                cerr << "KeyClient : error get key for " << setbase(10) << batchNumber << " chunks" << endl;
                return;
            } else {
                for (int i = 0; i < batchNumber; i++) {
                    memcpy(batchList[i].chunk.encryptKey, chunkKey + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
#if SYSTEM_DEBUG_FLAG == 1
                    cerr << "KeyClient : chunk " << batchList[i].chunk.ID << ", encrypt key = " << endl;
                    PRINT_BYTE_ARRAY_KEY_CLIENT(stdout, batchList[i].chunk.encryptKey, 32);
#endif
#if ENCODER_MODULE_ENABLED == 1
                    encoderObj_->insertMQ(batchList[i]);
#else
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartKey, NULL);
#endif
                    u_char ciphertext[batchList[i].chunk.logicDataSize];
                    bool encryptChunkContentStatus = cryptoObj_->encryptWithKey(batchList[i].chunk.logicData, batchList[i].chunk.logicDataSize, batchList[i].chunk.encryptKey, ciphertext);
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendKey, NULL);
                    diff = 1000000 * (timeendKey.tv_sec - timestartKey.tv_sec) + timeendKey.tv_usec - timestartKey.tv_usec;
                    second = diff / 1000000.0;
                    chunkContentEncryptionTime += second;
#endif
                    if (!encryptChunkContentStatus) {
                        cerr << "KeyClient : cryptoPrimitive error, encrypt chunk logic data error" << endl;
                        return;
                    } else {
                        memcpy(batchList[i].chunk.logicData, ciphertext, batchList[i].chunk.logicDataSize);
                    }
#if ENCODER_MODULE_ENABLED == 1
                    encoderObj_->insertMQ(batchList[i]);
#else
                    powObj_->insertMQ(batchList[i]);
#endif

#endif
                }
                batchList.clear();
                memset(chunkHash, 0, CHUNK_HASH_SIZE * keyBatchSize_);
                memset(chunkKey, 0, CHUNK_ENCRYPT_KEY_SIZE * keyBatchSize_);
                batchNumber = 0;
            }
        }
        if (JobDoneFlag) {
#if ENCODER_MODULE_ENABLED == 1
            bool editJobDoneFlagStatus = encoderObj_->editJobDoneFlag();
#else
            bool editJobDoneFlagStatus = powObj_->editJobDoneFlag();
#endif
            if (!editJobDoneFlagStatus) {
                cerr << "KeyClient : error to set job done flag for encoder" << endl;
            }
            break;
        }
    }
#if SYSTEM_BREAK_DOWN == 1
#if FINGERPRINTER_MODULE_ENABLE == 0
    cout << "KeyClient : plain chunk hash generate time = " << generatePlainChunkHashTime << " s" << endl;
#endif
    cout << "KeyClient : key exchange encrypt work time = " << keyExchangeEncTime << " s" << endl;
    cout << "KeyClient : key generate total work time = " << keyGenTime << " s" << endl;
#if ENCODER_MODULE_ENABLED == 0
    cout << "KeyClient : chunk encryption work time = " << chunkContentEncryptionTime << " s" << endl;
#endif
#endif
#if KEY_GEN_SGX_CTR == 1
    ofstream counterOut;
    counterOut.open(keyGenFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << keyGenFileName << endl;
    } else {
        char writeBuffer[16];
        memcpy(writeBuffer, nonce, 12);
        memcpy(writeBuffer + 12, &counter, sizeof(uint32_t));
        counterOut.write(writeBuffer, 16);
        counterOut.close();
        cerr << "KeyClient : Stored current counter file : " << keyGenFileName << endl;
    }
#endif
    return;
}

#if KEY_GEN_SGX_CFB == 1
bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber + 32];
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    cryptoObj_->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    cryptoObj_->sha256Hmac(sendHash, CHUNK_HASH_SIZE * batchNumber, sendHash + CHUNK_HASH_SIZE * batchNumber, keyExchangeKey_, 32);
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "KeyClient : send key exchange hmac = " << endl;
    PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, sendHash + CHUNK_HASH_SIZE * batchNumber, 32);
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber + 32)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber + 32];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char hmac[32];
    cryptoObj_->sha256Hmac(recvBuffer, CHUNK_HASH_SIZE * batchNumber, hmac, keyExchangeKey_, 32);
    if (memcmp(hmac, recvBuffer + batchNumber * CHUNK_HASH_SIZE, 32) != 0) {
        cerr << "KeyClient : recved keys hmac error" << endl;
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : recv key exchange hmac = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, recvBuffer + CHUNK_HASH_SIZE * batchNumber, 32);
        cerr << "KeyClient : client computed key exchange hmac = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, hmac, 32);
#endif
        return false;
    }
    cryptoObj_->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    return true;
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber + 32];
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    cryptoObj->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    cryptoObj->sha256Hmac(sendHash, CHUNK_HASH_SIZE * batchNumber, sendHash + CHUNK_HASH_SIZE * batchNumber, keyExchangeKey_, 32);
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "KeyClient : send key exchange hmac = " << endl;
    PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, sendHash + CHUNK_HASH_SIZE * batchNumber, 32);
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    mutexkeyGenerateSimulatorEncTime_.lock();
    keyExchangeEncTime += second;
    mutexkeyGenerateSimulatorEncTime_.unlock();
#endif
    if (!securityChannel->send(sslConnection, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber + 32)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber + 32];
    int recvSize;
    if (!securityChannel->recv(sslConnection, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char hmac[32];
    cryptoObj->sha256Hmac(recvBuffer, CHUNK_HASH_SIZE * batchNumber, hmac, keyExchangeKey_, 32);
    if (memcmp(hmac, recvBuffer + batchNumber * CHUNK_HASH_SIZE, 32) != 0) {
        cerr << "KeyClient : recved keys hmac error" << endl;
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "KeyClient : recv key exchange hmac = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, recvBuffer + CHUNK_HASH_SIZE * batchNumber, 32);
        cerr << "KeyClient : client computed key exchange hmac = " << endl;
        PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, hmac, 32);
#endif
        return false;
    }
    cryptoObj->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    mutexkeyGenerateSimulatorEncTime_.lock();
    keyExchangeEncTime += second;
    mutexkeyGenerateSimulatorEncTime_.unlock();
#endif
    return true;
}

#elif KEY_GEN_SGX_CTR == 1

bool KeyClient::keyExchangeXOR(u_char* result, u_char* input, u_char* xorBase, int batchNumber)
{
    for (int i = 0; i < batchNumber * CHUNK_HASH_SIZE; i++) {
        result[i] = input[i] ^ xorBase[i];
    }
    return true;
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, u_char* nonce, uint32_t counter, NetworkHeadStruct_t netHead)
{
    int sendSize = sizeof(NetworkHeadStruct_t) + CHUNK_HASH_SIZE * batchNumber + 32;
    u_char sendHash[sendSize];
    netHead.dataSize = batchNumber;
    memcpy(sendHash, &netHead, sizeof(NetworkHeadStruct_t));
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char keyExchangeXORBase[batchNumber * CHUNK_HASH_SIZE * 2];
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce, counter, batchNumber * 4, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "key exchange mask = " << endl;
    PRINT_BYTE_ARRAY_KEY_CLIENT(stderr, keyExchangeXORBase, batchNumber * CHUNK_HASH_SIZE * 2);
#endif
    keyExchangeXOR(sendHash + sizeof(NetworkHeadStruct_t), batchHashList, keyExchangeXORBase, batchNumber);
    cryptoObj_->sha256Hmac(sendHash + sizeof(NetworkHeadStruct_t), CHUNK_HASH_SIZE * batchNumber, sendHash + sizeof(NetworkHeadStruct_t) + CHUNK_HASH_SIZE * batchNumber, keyExchangeKey_, 32);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, sendSize)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber + 32];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char hmac[32];
    cryptoObj_->sha256Hmac(recvBuffer, CHUNK_HASH_SIZE * batchNumber, hmac, keyExchangeKey_, 32);
    if (memcmp(hmac, recvBuffer + batchNumber * CHUNK_HASH_SIZE, 32) != 0) {
        cerr << "KeyClient : recved keys hmac error" << endl;
        return false;
    }
    keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase + batchNumber * CHUNK_HASH_SIZE, batchNumber);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    return true;
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj, u_char* nonce, uint32_t counter, NetworkHeadStruct_t netHead)
{
    int sendSize = sizeof(NetworkHeadStruct_t) + CHUNK_HASH_SIZE * batchNumber + 32;
    u_char sendHash[sendSize];
    netHead.dataSize = batchNumber;
    memcpy(sendHash, &netHead, sizeof(NetworkHeadStruct_t));
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char keyExchangeXORBase[batchNumber * CHUNK_HASH_SIZE * 2];
    cryptoObj->keyExchangeCTRBaseGenerate(nonce, counter, batchNumber * 4, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
    keyExchangeXOR(sendHash + sizeof(NetworkHeadStruct_t), batchHashList, keyExchangeXORBase, batchNumber);
    cryptoObj->sha256Hmac(sendHash + sizeof(NetworkHeadStruct_t), CHUNK_HASH_SIZE * batchNumber, sendHash + sizeof(NetworkHeadStruct_t) + CHUNK_HASH_SIZE * batchNumber, keyExchangeKey_, 32);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    mutexkeyGenerateSimulatorEncTime_.lock();
    keyExchangeEncTime += second;
    mutexkeyGenerateSimulatorEncTime_.unlock();
#endif
    if (!securityChannel->send(sslConnection, (char*)sendHash, sendSize)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber + 32];
    int recvSize;
    if (!securityChannel->recv(sslConnection, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char hmac[32];
    cryptoObj->sha256Hmac(recvBuffer, CHUNK_HASH_SIZE * batchNumber, hmac, keyExchangeKey_, 32);
    if (memcmp(hmac, recvBuffer + batchNumber * CHUNK_HASH_SIZE, 32) != 0) {
        cerr << "KeyClient : recved keys hmac error" << endl;
        return false;
    }
    keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase + batchNumber * CHUNK_HASH_SIZE, batchNumber);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    second = diff / 1000000.0;
    mutexkeyGenerateSimulatorEncTime_.lock();
    keyExchangeEncTime += second;
    mutexkeyGenerateSimulatorEncTime_.unlock();
#endif
    return true;
}
#elif KEY_GEN_SERVER_MLE_NO_OPRF == 1
bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    if (!keySecurityChannel_->send(sslConnection_, (char*)batchHashList, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!keySecurityChannel_->recv(sslConnection_, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "KeyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
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

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection)
{
    if (!securityChannel->send(sslConnection, (char*)batchHashList, CHUNK_HASH_SIZE * batchNumber)) {
        cerr << "KeyClient: send socket error" << endl;
        return false;
    }
    u_char recvBuffer[CHUNK_ENCRYPT_KEY_SIZE * batchNumber];
    int recvSize;
    if (!securityChannel->recv(sslConnection, (char*)recvBuffer, recvSize)) {
        cerr << "KeyClient: recv socket error" << endl;
        return false;
    }
    if (recvSize % CHUNK_ENCRYPT_KEY_SIZE != 0) {
        cerr << "KeyClient: recv size % CHUNK_ENCRYPT_KEY_SIZE not equal to 0" << endl;
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

bool KeyClient::insertMQ(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool KeyClient::extractMQ(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool KeyClient::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}
