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

KeyClient::KeyClient(u_char* keyExchangeKey, uint64_t keyGenNumber)
{
    inputMQ_ = new messageQueue<Data_t>;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keyGenNumber_ = keyGenNumber;
    clientID_ = config.getClientID();
#if KEY_GEN_SGX_CTR == 1
    memset(nonce, 1, CRYPTO_BLOCK_SZIE - sizeof(uint32_t));
#endif
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
#if KEY_GEN_SGX_CTR == 1
    uint32_t counter = 0;
    //read old counter
    string counterFileName = ".CounterStore" + to_string(clientID);
    ifstream counterIn;
    counterIn.open(counterFileName, std::ifstream::in | std::ifstream::binary);
    if (!counterIn.is_open()) {
        cerr << "KeyClient : Can not open old counter file : \"" << counterFileName << "\", Directly reset counter to 0" << endl;
    } else {
        counterIn.seekg(0, ios_base::end);
        int counterFileSize = counterIn.tellg();
        counterIn.seekg(0, ios_base::beg);
        if (counterFileSize != sizeof(uint32_t)) {
            cerr << "KeyClient : stored old counter file size error" << endl;
        } else {
            char readBuffer[sizeof(uint32_t)];
            counterIn.read(readBuffer, sizeof(uint32_t));
            counterIn.close();
            if (counterIn.gcount() != sizeof(uint32_t)) {
                cerr << "KeyClient : read old counter file size error" << endl;
            } else {
                memcpy(&counter, readBuffer, sizeof(uint32_t));
                cout << "KeyClient : Read old counter file : " << counterFileName << " success, the original counter = " << counter << endl;
            }
        }
    }
    // cout << "KeyClient : Read old counter file : " << counterFileName << " success, the original counter = " << counter << endl;
    // done
    NetworkHeadStruct_t initHead, dataHead;
    initHead.clientID = clientID;
    initHead.dataSize = 16;
    initHead.messageType = KEY_GEN_UPLOAD_CLIENT_INFO;
    dataHead.clientID = clientID;
    dataHead.messageType = KEY_GEN_UPLOAD_CHUNK_HASH;
    char initInfoBuffer[sizeof(NetworkHeadStruct_t) + initHead.dataSize]; // clientID & nonce & counter
    memcpy(initInfoBuffer, &initHead, sizeof(NetworkHeadStruct_t));
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t), &counter, sizeof(uint32_t));
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t) + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
    if (!keySecurityChannel->send(sslConnection, initInfoBuffer, sizeof(NetworkHeadStruct_t) + initHead.dataSize)) {
        cerr << "KeyClient: send init information error" << endl;
        return;
    } else {
        cout << "KeyClient : send init information success, start key generate" << endl;
    }
#endif
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
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, keySecurityChannel, sslConnection, cryptoObj, counter, dataHead);
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
    counterOut.open(counterFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << counterFileName << endl;
    } else {
        char writeBuffer[sizeof(uint32_t)];
        memcpy(writeBuffer, &counter, sizeof(uint32_t));
        counterOut.write(writeBuffer, sizeof(uint32_t));
        counterOut.close();
        cout << "KeyClient : Stored current counter file : " << counterFileName << ", counter = " << counter << endl;
    }
#endif
#if SYSTEM_BREAK_DOWN == 1
    cerr << "KeyClient : client ID = " << clientID << endl;
    cerr << "KeyClient : key generate work time = " << keyGenTime << " s, total key generated is " << currentKeyGenNumber << endl;
    cerr << "KeyClient : key exchange work time = " << keyExchangeTime << " s, chunk hash generate time is " << chunkHashGenerateTime << " s" << endl;
#endif
    // delete cryptoObj;
    // delete keySecurityChannel;
    return;
}

void KeyClient::run()
{

#if SYSTEM_BREAK_DOWN == 1
    double keyGenTime = 0;
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
#if KEY_GEN_SGX_CTR == 1
    uint32_t counter = 0;
    //read old counter
    string counterFileName = ".CounterStore";
    ifstream counterIn;
    counterIn.open(counterFileName, std::ifstream::in | std::ifstream::binary);
    if (!counterIn.is_open()) {
        cerr << "KeyClient : Can not open old counter file : \"" << counterFileName << "\", Directly reset counter to 0" << endl;
    } else {
        counterIn.seekg(0, ios_base::end);
        int counterFileSize = counterIn.tellg();
        counterIn.seekg(0, ios_base::beg);
        if (counterFileSize != sizeof(uint32_t)) {
            cerr << "KeyClient : stored old counter file size error" << endl;
        } else {
            char readBuffer[sizeof(uint32_t)];
            counterIn.read(readBuffer, sizeof(uint32_t));
            counterIn.close();
            if (counterIn.gcount() != sizeof(uint32_t)) {
                cerr << "KeyClient : read old counter file size error" << endl;
            } else {
                memcpy(&counter, readBuffer, sizeof(uint32_t));
                cout << "KeyClient : Read old counter file : " << counterFileName << " success, the original counter = " << counter << endl;
            }
        }
    }
    // cout << "KeyClient : Read old counter file : " << counterFileName << " success, the original counter = " << counter << endl;
    // done
    NetworkHeadStruct_t initHead, dataHead;
    initHead.clientID = clientID_;
    initHead.dataSize = 16;
    initHead.messageType = KEY_GEN_UPLOAD_CLIENT_INFO;
    dataHead.clientID = clientID_;
    dataHead.messageType = KEY_GEN_UPLOAD_CHUNK_HASH;
    char initInfoBuffer[sizeof(NetworkHeadStruct_t) + initHead.dataSize]; // clientID & nonce & counter
    memcpy(initInfoBuffer, &initHead, sizeof(NetworkHeadStruct_t));
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t), &counter, sizeof(uint32_t));
    memcpy(initInfoBuffer + sizeof(NetworkHeadStruct_t) + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
    if (!keySecurityChannel_->send(sslConnection_, initInfoBuffer, sizeof(NetworkHeadStruct_t) + initHead.dataSize)) {
        cerr << "KeyClient: send init information error" << endl;
        return;
    } else {
        cout << "KeyClient : send init information success, start key generate" << endl;
    }

#endif
    while (true) {

        Data_t tempChunk;
        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            cerr << "KeyClient : Chunker jobs done, queue is empty" << endl;
            JobDoneFlag = true;
        }
        if (extractMQ(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                encoderObj_->insertMQ(tempChunk);
                continue;
            }
            batchList.push_back(tempChunk);
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
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, counter, dataHead);
            counter += batchNumber * 4;
#else
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize);
#endif

#if SYSTEM_BREAK_DOWN == 1
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
                    // memcpy(batchList[i].chunk.encryptKey, batchList[i].chunk.chunkHash, CHUNK_ENCRYPT_KEY_SIZE);
                    encoderObj_->insertMQ(batchList[i]);
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
#if SYSTEM_BREAK_DOWN == 1
    cout << "KeyClient : key exchange encrypt work time = " << keyExchangeEncTime << " s" << endl;
    cout << "KeyClient : key exchange total work time = " << keyExchangeTime << " s" << endl;
    cout << "KeyClient : keyGen total work time = " << keyGenTime << " s" << endl;
#endif
#if KEY_GEN_SGX_CTR == 1
    ofstream counterOut;
    counterOut.open(counterFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << counterFileName << endl;
    } else {
        char writeBuffer[sizeof(uint32_t)];
        memcpy(writeBuffer, &counter, sizeof(uint32_t));
        counterOut.write(writeBuffer, sizeof(uint32_t));
        counterOut.close();
        cout << "KeyClient : Stored current counter file : " << counterFileName << ", counter = " << counter << endl;
    }
#endif
    return;
}

#if KEY_GEN_SGX_CFB == 1
bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    cryptoObj_->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    memcpy(sendHash, batchHashList, batchNumber * CHUNK_HASH_SIZE);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber)) {
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
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartKey_enc, NULL);
#endif
        cryptoObj_->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
        memcpy(batchKeyList, recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendKey_enc, NULL);
        diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
        second = diff / 1000000.0;
        keyExchangeEncTime += second;
#endif
        return true;
    } else {
        return false;
    }
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    cryptoObj->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);
    if (!securityChannel->send(sslConnection, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber)) {
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
        cryptoObj->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
        return true;
    } else {
        return false;
    }
}

#elif KEY_GEN_SGX_CTR == 1

bool KeyClient::keyExchangeXOR(u_char* result, u_char* input, u_char* xorBase, int batchNumber)
{
    for (int i = 0; i < batchNumber * CHUNK_HASH_SIZE; i++) {
        result[i] = input[i] ^ xorBase[i];
    }
    return true;
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, uint32_t counter, NetworkHeadStruct_t netHead)
{
    u_char sendHash[sizeof(NetworkHeadStruct_t) + CHUNK_HASH_SIZE * batchNumber];
    memcpy(sendHash, &netHead, sizeof(NetworkHeadStruct_t));
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartKey_enc;
    struct timeval timeendKey_enc;
    gettimeofday(&timestartKey_enc, NULL);
#endif
    u_char keyExchangeXORBase[batchNumber * CHUNK_HASH_SIZE * 2];
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce, counter, batchNumber * 4, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
    keyExchangeXOR(sendHash + sizeof(NetworkHeadStruct_t), batchHashList, keyExchangeXORBase, batchNumber);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKey_enc, NULL);
    long diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
    double second = diff / 1000000.0;
    keyExchangeEncTime += second;
#endif
    if (!keySecurityChannel_->send(sslConnection_, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber + sizeof(NetworkHeadStruct_t))) {
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
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartKey_enc, NULL);
#endif
        keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase + batchNumber * CHUNK_HASH_SIZE, batchkeyNumber);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendKey_enc, NULL);
        diff = 1000000 * (timeendKey_enc.tv_sec - timestartKey_enc.tv_sec) + timeendKey_enc.tv_usec - timestartKey_enc.tv_usec;
        second = diff / 1000000.0;
        keyExchangeEncTime += second;
#endif
        return true;
    } else {
        return false;
    }
}

bool KeyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj, uint32_t counter, NetworkHeadStruct_t netHead)
{
    u_char sendHash[sizeof(NetworkHeadStruct_t) + CHUNK_HASH_SIZE * batchNumber];
    memcpy(sendHash, &netHead, sizeof(NetworkHeadStruct_t));
    u_char keyExchangeXORBase[batchNumber * CHUNK_HASH_SIZE * 2];
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce, counter, batchNumber * 4, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
    keyExchangeXOR(sendHash + sizeof(NetworkHeadStruct_t), batchHashList, keyExchangeXORBase, batchNumber);

    if (!securityChannel->send(sslConnection, (char*)sendHash, CHUNK_HASH_SIZE * batchNumber + sizeof(NetworkHeadStruct_t))) {
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
        keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase + batchNumber * CHUNK_HASH_SIZE, batchkeyNumber);
        return true;
    } else {
        return false;
    }
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
