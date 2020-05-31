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
    clientID_ = config.getClientID();
}

keyClient::keyClient(u_char* keyExchangeKey, uint64_t keyGenNumber)
{
    inputMQ_ = new messageQueue<Data_t>;
    cryptoObj_ = new CryptoPrimitive();
    keyBatchSize_ = (int)config.getKeyBatchSize();
    memcpy(keyExchangeKey_, keyExchangeKey, KEY_SERVER_SESSION_KEY_SIZE);
    keyGenNumber_ = keyGenNumber;
    clientID_ = config.getClientID();
#ifdef SGX_KEY_GEN_CTR
    memset(nonce, 1, CRYPTO_BLOCK_SZIE - sizeof(uint32_t));
#endif
}

keyClient::~keyClient()
{
    if (cryptoObj_ != NULL) {
        delete cryptoObj_;
    }
    inputMQ_->~messageQueue();
    delete inputMQ_;
#ifdef BREAK_DOWN
    cout << "KeyClient : key exchange encryption time = " << keyExchangeEncTime << " s" << endl;
#endif
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
#ifdef SGX_KEY_GEN_CTR
    uint32_t counter = 0;
    //read old counter
    string counterFileName = ".CounterStore";
    ifstream counterIn;
    counterIn.open(counterFileName, std::ifstream::in | std::ifstream::binary);
    if (!counterIn.is_open()) {
        cerr << "KeyClient : Can not open old counter file : " << counterFileName << ". Directly reset counter to 0" << endl;
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
            }
        }
    }
    cout << "KeyClient : Read old counter file : " << counterFileName << " success, the original counter = " << counter << endl;
    // done
    char initInfoBuffer[16 + sizeof(int)]; // clientID & nonce & counter
    memcpy(initInfoBuffer, &clientID_, sizeof(int));
    memcpy(initInfoBuffer + sizeof(int), &counter, sizeof(uint32_t));
    memcpy(initInfoBuffer + sizeof(int) + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
    if (!keySecurityChannel->send(sslConnection, initInfoBuffer, 16 + sizeof(int))) {
        cerr << "keyClient: send init information error" << endl;
        return;
    } else {
        cout << "KeyClient : send init information success, start key generate" << endl;
    }
#endif
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
#ifdef SGX_KEY_GEN_CTR
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, keySecurityChannel, sslConnection, cryptoObj, counter);
            counter += batchNumber * 4;
#else
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, keySecurityChannel, sslConnection, cryptoObj);
#endif
#ifdef BREAK_DOWN
            gettimeofday(&timeendKeySimulator, NULL);
            diff = 1000000 * (timeendKeySimulator.tv_sec - timestartKeySimulator.tv_sec) + timeendKeySimulator.tv_usec - timestartKeySimulator.tv_usec;
            second = diff / 1000000.0;
            keyExchangeTime += second;
            keyGenTime += second;
#endif
#ifdef SGX_KEY_GEN_CTR
            counter += batchNumber * 4;
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
#ifdef SGX_KEY_GEN_CTR
    ofstream counterOut;
    counterOut.open(counterFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << counterFileName << endl;
    } else {
        char writeBuffer[sizeof(uint32_t)];
        memcpy(writeBuffer, &counter, sizeof(uint32_t));
        counterOut.write(writeBuffer, sizeof(uint32_t));
        counterOut.close();
        cout << "KeyClient : Stored current counter file : " << counterFileName << endl;
    }
#endif
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
#ifdef SGX_KEY_GEN_CTR
    uint32_t counter = 0;
    //read old counter
    string counterFileName = ".CounterStore";
    ifstream counterIn;
    counterIn.open(counterFileName, std::ifstream::in | std::ifstream::binary);
    if (!counterIn.is_open()) {
        cerr << "KeyClient : Can not open old counter file : " << counterFileName << ". Directly reset counter to 0" << endl;
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
            }
        }
    }
    cout << "KeyClient : Read old counter file : " << counterFileName << " success, the original counter = " << counter << endl;
    // done
    char initInfoBuffer[16 + sizeof(int)]; // clientID & nonce & counter
    memcpy(initInfoBuffer, &clientID_, sizeof(int));
    memcpy(initInfoBuffer + sizeof(int), &counter, sizeof(uint32_t));
    memcpy(initInfoBuffer + sizeof(int) + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
    if (!keySecurityChannel_->send(sslConnection_, initInfoBuffer, 16 + sizeof(int))) {
        cerr << "keyClient: send init information error" << endl;
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
            gettimeofday(&timeendKey, NULL);recvSize
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
#ifdef SGX_KEY_GEN_CTR
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize, counter);
            counter += batchNumber * 4;
#else
            bool keyExchangeStatus = keyExchange(chunkHash, batchNumber, chunkKey, batchedKeySize);
#endif

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
#ifdef SGX_KEY_GEN_CTR
    ofstream counterOut;
    counterOut.open(counterFileName, std::ofstream::out | std::ofstream::binary);
    if (!counterOut.is_open()) {
        cerr << "KeyClient : Can not open counter store file : " << counterFileName << endl;
    } else {
        char writeBuffer[sizeof(uint32_t)];
        memcpy(writeBuffer, &counter, sizeof(uint32_t));
        counterOut.write(writeBuffer, sizeof(uint32_t));
        counterOut.close();
        cout << "KeyClient : Stored current counter file : " << counterFileName << endl;
    }
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

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    // #ifdef BREAK_DOWN
    //     struct timeval timestartKey_enc;
    //     struct timeval timeendKey_enc;
    //     gettimeofday(&timestartKey_enc, NULL);
    // #endif
    cryptoObj->keyExchangeEncrypt(batchHashList, batchNumber * CHUNK_HASH_SIZE, keyExchangeKey_, keyExchangeKey_, sendHash);

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
        cryptoObj->keyExchangeDecrypt(recvBuffer, batchkeyNumber * CHUNK_ENCRYPT_KEY_SIZE, keyExchangeKey_, keyExchangeKey_, batchKeyList);
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

bool keyClient::keyExchangeXOR(u_char* result, u_char* input, u_char* xorBase, int batchNumber)
{
    for (int i = 0; i < batchNumber * CHUNK_HASH_SIZE; i++) {
        result[i] = input[i] ^ xorBase[i];
    }
    return true;
}

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, uint32_t counter)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    // #ifdef BREAK_DOWN
    //     struct timeval timestartKey_enc;
    //     struct timeval timeendKey_enc;
    //     gettimeofday(&timestartKey_enc, NULL);
    // #endif

    u_char keyExchangeXORBase[batchNumber * CHUNK_HASH_SIZE * 2];
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce, counter, batchNumber * 4, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
    keyExchangeXOR(sendHash, batchHashList, keyExchangeXORBase, batchNumber);

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

        keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase + batchNumber * CHUNK_HASH_SIZE, batchkeyNumber);

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

bool keyClient::keyExchange(u_char* batchHashList, int batchNumber, u_char* batchKeyList, int& batchkeyNumber, ssl* securityChannel, SSL* sslConnection, CryptoPrimitive* cryptoObj, uint32_t counter)
{
    u_char sendHash[CHUNK_HASH_SIZE * batchNumber];
    // #ifdef BREAK_DOWN
    //     struct timeval timestartKey_enc;
    //     struct timeval timeendKey_enc;
    //     gettimeofday(&timestartKey_enc, NULL);
    // #endif
    u_char keyExchangeXORBase[batchNumber * CHUNK_HASH_SIZE];
    cryptoObj_->keyExchangeCTRBaseGenerate(nonce, counter, batchNumber * 2, keyExchangeKey_, keyExchangeKey_, keyExchangeXORBase);
    keyExchangeXOR(sendHash, batchHashList, keyExchangeXORBase, batchNumber);
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
        keyExchangeXOR(batchKeyList, recvBuffer, keyExchangeXORBase, batchkeyNumber);
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
