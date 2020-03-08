#include "keyServer.hpp"
#include <sys/time.h>
#ifdef NON_OPRF
#include <openssl/sha.h>
#endif
extern Configure config;

struct timeval timestart;
struct timeval timeend;

void PRINT_BYTE_ARRAY_KEY_SERVER(
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

keyServer::keyServer(ssl* keyServerSecurityChannelTemp)
{
    rsa_ = RSA_new();
    key_ = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(key_, &rsa_, NULL, passwd);
#ifdef OPENSSL_V_1_0_2
    keyD_ = rsa_->d;
    keyN_ = rsa_->n;
#else
    RSA_get0_key(rsa_, &keyN_, nullptr, &keyD_);
#endif
    u_char keydBuffer[4096];
    int lenKeyd;
    lenKeyd = BN_bn2bin(keyD_, keydBuffer);
    string keyd((char*)keydBuffer, lenKeyd / 2);
    client = new kmClient(keyd);
    clientThreadCount_ = 0;
    keyGenerateCount_ = 0;
    keyGenLimitPerSessionKey_ = config.getKeyGenLimitPerSessionkeySize();
    raRequestFlag = true;
    keySecurityChannel_ = keyServerSecurityChannelTemp;
    sessionKeyUpdateCount_ = config.getKeyRegressionMaxTimes();
}

keyServer::~keyServer()
{
    BIO_free_all(key_);
    RSA_free(rsa_);
    delete client;
    delete keySecurityChannel_;
}

bool keyServer::doRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection)
{
    if (!client->init(raSecurityChannel, sslConnection)) {
        cerr << "keyServer : enclave not truster" << endl;
        return false;
    } else {
        cout << "KeyServer : enclave trusted" << endl;
    }
    multiThreadCountMutex_.lock();
    keyGenerateCount_ = 0;
    multiThreadCountMutex_.unlock();
    return true;
}

void keyServer::runRAwithSPRequest()
{
    ssl* raSecurityChannelTemp = new ssl(config.getKeyServerIP(), config.getkeyServerRArequestPort(), SERVERSIDE);
    char recvBuffer[sizeof(NetworkHeadStruct_t)];
    int recvSize;
    while (true) {
        SSL* sslConnection = raSecurityChannelTemp->sslListen().second;
        raSecurityChannelTemp->recv(sslConnection, recvBuffer, recvSize);
        raRequestFlag = true;
        cout << "KeyServer : recv storage server ra request, waiting for ra now" << endl;
        free(sslConnection);
    }
}

void keyServer::runRA()
{
    while (true) {
        if (raRequestFlag) {
            while (!(clientThreadCount_ == 0))
                ;
            cout << "KeyServer : start do remote attestation to storage server" << endl;
            keyGenerateCount_ = 0;
            ssl* raSecurityChannelTemp = new ssl(config.getStorageServerIP(), config.getKMServerPort(), CLIENTSIDE);
            SSL* sslConnection = raSecurityChannelTemp->sslConnect().second;
            bool remoteAttestationStatus = doRemoteAttestation(raSecurityChannelTemp, sslConnection);
            if (remoteAttestationStatus) {
                delete raSecurityChannelTemp;
                free(sslConnection);
                raRequestFlag = false;
                cout << "KeyServer : do remote attestation to storage SP done" << endl;
            } else {
                delete raSecurityChannelTemp;
                free(sslConnection);
                cerr << "KeyServer : do remote attestation to storage SP error" << endl;
                continue;
            }
        }
    }
}

void keyServer::runSessionKeyUpdate()
{
    while (true) {
        if (keyGenerateCount_ >= keyGenLimitPerSessionKey_) {
            while (!(clientThreadCount_ == 0))
                ;
            cout << "KeyServer : start session key update with storage server" << endl;
            keyGenerateCount_ = 0;
            ssl* raSecurityChannelTemp = new ssl(config.getStorageServerIP(), config.getKMServerPort(), CLIENTSIDE);
            SSL* sslConnection = raSecurityChannelTemp->sslConnect().second;
            bool enclaveSessionKeyUpdateStatus = client.sessionKeyUpdate();
            if (remoteAttestationStatus) {
                char sendBuffer[sizeof(NetworkHeadStruct_t)];
                int sendSize;
                NetworkHeadStruct_t requestHeader;
                requestHeader.messageType = KEY_SERVER_SESSION_KEY_UPDATE;
                memcpy(sendBuffer, &requestHeader, sizeof(requestHeader));
                raSecurityChannelTemp->send(sslConnection, sendBuffer, sendSize);
                delete raSecurityChannelTemp;
                free(sslConnection);
                cout << "KeyServer : update session key storage SP done" << endl;
            } else {
                delete raSecurityChannelTemp;
                free(sslConnection);
                cerr << "KeyServer : update session key with storage SP error" << endl;
                continue;
            }
        }
    }
}

#ifdef SGX_KEY_GEN
void keyServer::run(SSL* connection)
{
#ifdef BREAK_DOWN
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount_++;
    multiThreadCountMutex_.unlock();
    uint64_t currentThreadkeyGenerationNumber = 0;
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }

        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        cerr << "KeyServer : recv hash number = " << recvNumber << endl;
        u_char key[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        multiThreadMutex_.lock();

#ifdef BREAK_DOWN
        gettimeofday(&timestart, 0);
#endif
        client->request(hash, recvSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE);
#ifdef BREAK_DOWN
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();
        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }
    }
}

#elif NO_OPRF

void keyServer::run(SSL* connection)
{
#ifdef BREAK_DOWN
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount_++;
    multiThreadCountMutex_.unlock();
    uint64_t currentThreadkeyGenerationNumber = 0;
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }
        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        cerr << "KeyServer : recv hash number = " << recvNumber << endl;

        u_char key[config.getKeyBatchSize() * CHUNK_HASH_SIZE];

        multiThreadMutex_.lock();
#ifdef BREAK_DOWN
        gettimeofday(&timestart, 0);
#endif
        for (int i = 0; i < recvNumber; i++) {
            u_char tempKeySeed[CHUNK_HASH_SIZE + 64];
            memset(tempKeySeed, 0, CHUNK_HASH_SIZE + 64);
            memcpy(tempKeySeed, hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
            SHA256(tempKeySeed, CHUNK_HASH_SIZE, key + i * CHUNK_ENCRYPT_KEY_SIZE);
        }
#ifdef BREAK_DOWN
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();
        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }
    }
}

#elif OPRF

void keyServer::run(SSL* connection)
{
#ifdef BREAK_DOWN
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount_++;
    multiThreadCountMutex_.unlock();
    uint64_t currentThreadkeyGenerationNumber = 0;
    int OPRFResultSize = 128;
    while (true) {
        u_char hash[config.getKeyBatchSize() * OPRFResultSize];
        int recvSize = 0;
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }
        int recvNumber = recvSize / OPRFResultSize;
        cerr << "KeyServer : recv hash number = " << recvNumber << endl;

        u_char key[config.getKeyBatchSize() * OPRFResultSize];

        multiThreadMutex_.lock();
#ifdef BREAK_DOWN
        gettimeofday(&timestart, 0);
#endif
        for (int i = 0; i < recvNumber; i++) {
            u_char tempKeySeed[CHUNK_HASH_SIZE + 64];
            memset(tempKeySeed, 0, CHUNK_HASH_SIZE + 64);
            memcpy(tempKeySeed, hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
            SHA256(tempKeySeed, CHUNK_HASH_SIZE, key + i * CHUNK_ENCRYPT_KEY_SIZE);
        }
#ifdef BREAK_DOWN
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();
        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * OPRFResultSize)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }
    }
}
#endif