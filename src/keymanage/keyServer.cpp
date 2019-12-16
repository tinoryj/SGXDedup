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

keyServer::keyServer()
{
    rsa_ = RSA_new();
    key_ = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(key_, &rsa_, NULL, passwd);
    RSA_get0_key(rsa_, &keyN_, nullptr, &keyD_);
    u_char keydBuffer[4096];
    int lenKeyd;
    lenKeyd = BN_bn2bin(keyD_, keydBuffer);
    string keyd((char*)keydBuffer, lenKeyd / 2);
    client = new kmClient(keyd);
    clientThreadCount = 0;
    keyGenerateCount = 0;
    keyGenLimitPerSessionKey_ = config.getKeyGenLimitPerSessionkeySize();
    raRequestFlag = false;
}

keyServer::~keyServer()
{
    BIO_free_all(key_);
    RSA_free(rsa_);
    delete client;
}

bool keyServer::doRemoteAttestation(Socket socket)
{
    if (!client->init(socket)) {
        cerr << "keyServer : enclave not truster" << endl;
        return false;
    } else {
        cout << "KeyServer : enclave trusted" << endl;
    }
    multiThreadCountMutex_.lock();
    keyGenerateCount = 0;
    multiThreadCountMutex_.unlock();
    return true;
}

void keyServer::runRAwithSPRequest()
{
    Socket socketRARequest(SERVER_TCP, "", config.getkeyServerRArequestPort());
    u_char recvBuffer[sizeof(NetworkHeadStruct_t)];
    int recvSize;
    while (true) {
        Socket raRequestTempSocket = socketRARequest.Listen();
        raRequestTempSocket.Recv(recvBuffer, recvSize);
        raRequestFlag = true;
        cout << "KeyServer : recv storage server ra request, waiting for ra now" << endl;
        raRequestTempSocket.finish();
    }
}

void keyServer::runRA()
{
    while (true) {
        if (keyGenerateCount >= keyGenLimitPerSessionKey_ || raRequestFlag) {
            while (!(clientThreadCount == 0))
                ;
            cout << "KeyServer : start do remote attestation to storage server" << endl;
            keyGenerateCount = 0;
            Socket tmpServerSocket;
            tmpServerSocket.init(CLIENT_TCP, config.getStorageServerIP(), config.getKMServerPort());
            if (doRemoteAttestation(tmpServerSocket)) {
                tmpServerSocket.finish();
                raRequestFlag = false;
                cout << "KeyServer : do remote attestation to storage SP done" << endl;
            } else {
                tmpServerSocket.finish();
                cerr << "KeyServer : do remote attestation to storage SP error" << endl;
                return;
            }
        }
    }
}

void keyServer::run(Socket socket)
{
#ifdef BREAK_DOWN
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount++;
    multiThreadCountMutex_.unlock();
    uint64_t currentThreadkeyGenerationNumber = 0;
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!socket.Recv(hash, recvSize)) {
            socket.finish();
            multiThreadCountMutex_.lock();
            clientThreadCount--;
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
            clientThreadCount--;
            multiThreadCountMutex_.unlock();
            socket.finish();
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
#ifdef NON_OPRF
        for (int i = 0; i < recvNumber; i++) {
            u_char tempKeySeed[CHUNK_HASH_SIZE + 64];
            memset(tempKeySeed, 0, CHUNK_HASH_SIZE + 64);
            memcpy(tempKeySeed, hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
            SHA256(tempKeySeed, CHUNK_HASH_SIZE, key + i * CHUNK_ENCRYPT_KEY_SIZE);
        }
#endif
#ifdef BREAK_DOWN
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount += recvNumber;
        multiThreadMutex_.unlock();
        currentThreadkeyGenerationNumber += recvNumber;
        if (!socket.Send(key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            multiThreadCountMutex_.lock();
            clientThreadCount--;
            multiThreadCountMutex_.unlock();
            socket.finish();
#ifdef BREAK_DOWN
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
#endif
            return;
        }
    }
}