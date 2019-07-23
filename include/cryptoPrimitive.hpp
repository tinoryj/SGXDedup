#ifndef GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
#define GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
#include "configure.hpp"
#include "dataStructure.hpp"
#include <bits/stdc++.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/cmac.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/opensslconf.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <pthread.h>

#define OPENSSL_VERSION_1_1 1
#define OPENSSL_THREAD_DEFINES

using namespace std;

typedef struct {
    pthread_mutex_t* lockList;
    long* cntList;
} opensslLock_t;

class CryptoPrimitive {
private:
    const EVP_MD* md_;

    int hashSize_ = CHUNK_HASH_SIZE;
    int keySize_ = CHUNK_ENCRYPT_KEY_SIZE;
    int blockSize_ = CRYPTO_BLOCK_SZIE;

    EVP_CIPHER_CTX* cipherctx_ = EVP_CIPHER_CTX_new();
    EVP_MD_CTX* mdctx_ = EVP_MD_CTX_new();
    const EVP_CIPHER* cipher_;
    u_char* iv_;
    u_char* chunkKeyEncryptionKey_;

    static opensslLock_t* opensslLock_;
    static void opensslLockingCallback_(int mode, int type, const char* file, int line);
    static void opensslThreadID_(CRYPTO_THREADID* id);

public:
    CryptoPrimitive();
    ~CryptoPrimitive();
    static bool opensslLockSetup();
    static bool opensslLockCleanup();
    bool generateHash(u_char* dataBuffer, const int& dataSize, u_char* hash);
    bool encryptWithKey(u_char* dataBuffer, const int& dataSize, u_char* key, u_char* ciphertext);
    bool decryptWithKey(u_char* ciphertext, const int& dataSize, u_char* key, u_char* dataBuffer);
    bool keyExchangeEncrypt(u_char* dataBuffer, const int& dataSize, u_char* key, u_char* iv, u_char* ciphertext);
    bool keyExchangeDecrypt(u_char* ciphertext, const int& dataSize, u_char* key, u_char* iv, u_char* dataBuffer);
    bool encryptChunk(Chunk_t& chunk);
    bool decryptChunk(Chunk_t& chunk);
};

#endif //GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP