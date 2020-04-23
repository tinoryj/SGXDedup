#include "cryptoPrimitive.hpp"

/*initialize the static variable*/
opensslLock_t* CryptoPrimitive::opensslLock_ = NULL;

/*
 * OpenSSL locking callback function
 */
void CryptoPrimitive::opensslLockingCallback_(int mode, int type, const char* file, int line)
{
    if (mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(opensslLock_->lockList[type]));
        CryptoPrimitive::opensslLock_->cntList[type]++;
    } else {
        pthread_mutex_unlock(&(opensslLock_->lockList[type]));
    }
}

/*
 * get the id of the current thread
 *
 * @param id - the thread id <return>
 */
void CryptoPrimitive::opensslThreadID_(CRYPTO_THREADID* id)
{
    CRYPTO_THREADID_set_numeric(id, pthread_self());
}

/*
 * set up OpenSSL locks
 *
 * @return - a boolean value that indicates if the setup succeeds
 */
bool CryptoPrimitive::opensslLockSetup()
{
#if defined(OPENSSL_THREADS)

    opensslLock_ = (opensslLock_t*)malloc(sizeof(opensslLock_t));

    opensslLock_->lockList = (pthread_mutex_t*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    opensslLock_->cntList = (long*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));

    for (int i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_init(&(opensslLock_->lockList[i]), NULL);
        opensslLock_->cntList[i] = 0;
    }

    CRYPTO_set_locking_callback(&opensslLockingCallback_);

    return true;
#else
    fprintf(stderr, "Error: OpenSSL was not configured with thread support!\n");

    return false;
#endif
}

/*
 * clean up OpenSSL locks
 *
 * @return - a boolean value that indicates if the cleanup succeeds
 */
bool CryptoPrimitive::opensslLockCleanup()
{
#if defined(OPENSSL_THREADS)

    CRYPTO_set_locking_callback(NULL);
    for (int i = 0; i < CRYPTO_num_locks(); i++) {
        pthread_mutex_destroy(&(opensslLock_->lockList[i]));
    }

    OPENSSL_free(opensslLock_->lockList);
    OPENSSL_free(opensslLock_->cntList);
    free(opensslLock_);

    fprintf(stderr, "OpenSSL lock cleanup done\n");

    return true;
#else
    fprintf(stderr, "Error: OpenSSL was not configured with thread support!\n");

    return false;
#endif
}

/*
 * constructor of CryptoPrimitive
 *
 * @param cryptoType - the type of CryptoPrimitive
 */
CryptoPrimitive::CryptoPrimitive()
{

#if defined(OPENSSL_THREADS)
    /*check if opensslLockSetup() has been called to set up OpenSSL locks*/
    opensslLockSetup();
    if (opensslLock_ == NULL) {
        fprintf(stderr, "Error: opensslLockSetup() was not called before initializing CryptoPrimitive instances\n");
        exit(1);
    }

    hashSize_ = CHUNK_HASH_SIZE;
    //int keySize_ = CHUNK_ENCRYPT_KEY_SIZE;
    blockSize_ = CRYPTO_BLOCK_SZIE;
    md_ = EVP_sha256();
    cipherctx_ = EVP_CIPHER_CTX_new();
    if (cipherctx_ == nullptr) {
        cerr << "can not initial cipher ctx" << endl;
    }
    cmacctx_ = CMAC_CTX_new();
    if (cmacctx_ == nullptr) {
        cerr << "error initial cmac ctx" << endl;
    }
#ifdef OPENSSL_V_1_0_2
    mdctx_ = EVP_MD_CTX_create();
#else
    mdctx_ = EVP_MD_CTX_new();
#endif
    EVP_MD_CTX_init(mdctx_);
    EVP_CIPHER_CTX_init(cipherctx_);
    cipher_ = EVP_aes_256_cfb();
    iv_ = (u_char*)malloc(sizeof(u_char) * blockSize_);
    memset(iv_, 0, blockSize_);
    chunkKeyEncryptionKey_ = (u_char*)malloc(sizeof(u_char) * CHUNK_ENCRYPT_KEY_SIZE);
    memset(chunkKeyEncryptionKey_, 0, CHUNK_ENCRYPT_KEY_SIZE);

#else
    fprintf(stderr, "Error: OpenSSL was not configured with thread support!\n");
    exit(1);
#endif
}
/*
 * destructor of CryptoPrimitive
 */
CryptoPrimitive::~CryptoPrimitive()
{
#ifdef OPENSSL_V_1_0_2
    EVP_MD_CTX_cleanup(mdctx_);
#else
    EVP_MD_CTX_reset(mdctx_);
#endif
    EVP_MD_CTX_destroy(mdctx_);
#ifdef OPENSSL_V_1_0_2
    EVP_CIPHER_CTX_free(cipherctx_);
    EVP_CIPHER_CTX_cleanup(cipherctx_);
#else
    EVP_CIPHER_CTX_reset(cipherctx_);
    EVP_CIPHER_CTX_cleanup(cipherctx_);
#endif
    free(iv_);
    free(chunkKeyEncryptionKey_);
    CMAC_CTX_cleanup(cmacctx_);
}

/*
 * generate the hash for the data stored in a buffer
 *
 * @param dataBuffer - the buffer that stores the data
 * @param dataSize - the size of the data
 * @param hash - the generated hash <return>
 *
 * @return - a boolean value that indicates if the hash generation succeeds
 */
bool CryptoPrimitive::generateHash(u_char* dataBuffer, const int dataSize, u_char* hash)
{
    if (EVP_DigestInit_ex(mdctx_, EVP_sha256(), nullptr) != 1) {
        cerr << "hash error\n";
        return false;
    }
    if (EVP_DigestUpdate(mdctx_, dataBuffer, dataSize) != 1) {
        cerr << "hash error\n";
        return false;
    }
    int hashSize;
    if (EVP_DigestFinal_ex(mdctx_, hash, (unsigned int*)&hashSize) != 1) {
        cerr << "hash error\n";
        return false;
    }
    return true;
}

bool CryptoPrimitive::encryptWithKey(u_char* dataBuffer, const int dataSize, u_char* key, u_char* ciphertext)
{
    int cipherlen, len;
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    if (EVP_EncryptInit_ex(cipherctx_, EVP_aes_256_cfb(), nullptr, key, iv_) != 1) {
        cerr << "encrypt error\n";
        return false;
    }
    if (EVP_EncryptUpdate(cipherctx_, ciphertext, &cipherlen, dataBuffer, dataSize) != 1) {
        cerr << "encrypt error\n";
        return false;
    }
    if (EVP_EncryptFinal_ex(cipherctx_, ciphertext + cipherlen, &len) != 1) {
        cerr << "encrypt error\n";
        return false;
    }
    cipherlen += len;
    if (cipherlen != dataSize) {
        cerr << "CryptoPrimitive : encrypt output size not equal to origin size" << endl;
        return false;
    }
    return true;
}

bool CryptoPrimitive::decryptWithKey(u_char* ciphertext, const int dataSize, u_char* key, u_char* dataBuffer)
{
    int plaintlen, len;
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    if (EVP_DecryptInit_ex(cipherctx_, EVP_aes_256_cfb(), nullptr, key, iv_) != 1) {
        cerr << "decrypt error\n";
        return false;
    }
    if (EVP_DecryptUpdate(cipherctx_, dataBuffer, &plaintlen, ciphertext, dataSize) != 1) {
        cerr << "decrypt error\n";
        return false;
    }
    if (EVP_DecryptFinal_ex(cipherctx_, dataBuffer + plaintlen, &len) != 1) {
        cerr << "decrypt error\n";
        return false;
    }
    plaintlen += len;
    if (plaintlen != dataSize) {
        cerr << "CryptoPrimitive : decrypt output size not equal to origin size" << endl;
        return false;
    }
    return true;
}

bool CryptoPrimitive::encryptChunk(Chunk_t& chunk)
{
    u_char ciphertext[chunk.logicDataSize];
    u_char cipherKey[CHUNK_ENCRYPT_KEY_SIZE];
    if (!encryptWithKey(chunk.logicData, chunk.logicDataSize, chunk.encryptKey, ciphertext)) {
        cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
        return false;
    } else {
        memcpy(chunk.logicData, ciphertext, chunk.logicDataSize);
        if (!encryptWithKey(chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE, chunkKeyEncryptionKey_, cipherKey)) {
            cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
            return false;
        } else {
            memcpy(chunk.encryptKey, cipherKey, CHUNK_ENCRYPT_KEY_SIZE);
            return true;
        }
    }
}

bool CryptoPrimitive::decryptChunk(Chunk_t& chunk)
{
    u_char plaintData[chunk.logicDataSize];
    u_char plaintKey[CHUNK_ENCRYPT_KEY_SIZE];
    if (!decryptWithKey(chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE, chunkKeyEncryptionKey_, plaintKey)) {
        cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
        return false;
    } else {
        if (!decryptWithKey(chunk.logicData, chunk.logicDataSize, plaintKey, plaintData)) {
            cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
            return false;
        } else {
            memcpy(chunk.logicData, plaintData, chunk.logicDataSize);
            return true;
        }
    }
}

bool CryptoPrimitive::decryptChunk(u_char* chunkData, int chunkSize, u_char* key, u_char* plaintData)
{
    u_char plaintKey[CHUNK_ENCRYPT_KEY_SIZE];
    if (!decryptWithKey(key, CHUNK_ENCRYPT_KEY_SIZE, chunkKeyEncryptionKey_, plaintKey)) {
        cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
        return false;
    } else {
        if (!decryptWithKey(chunkData, chunkSize, plaintKey, plaintData)) {
            cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
            return false;
        } else {
            return true;
        }
    }
}

bool CryptoPrimitive::keyExchangeDecrypt(u_char* ciphertext, const int dataSize, u_char* key, u_char* iv, u_char* dataBuffer)
{
    int plaintlen, len;
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    if (EVP_DecryptInit_ex(cipherctx_, EVP_aes_256_cfb(), nullptr, key, iv) != 1) {
        cerr << "decrypt error\n";
        return false;
    }

    if (EVP_DecryptUpdate(cipherctx_, dataBuffer, &plaintlen, ciphertext, dataSize) != 1) {
        cerr << "decrypt error\n";
        return false;
    }

    if (EVP_DecryptFinal_ex(cipherctx_, dataBuffer + plaintlen, &len) != 1) {
        cerr << "decrypt error\n";
        return false;
    }

    plaintlen += len;
    if (plaintlen != dataSize) {
        cerr << "CryptoPrimitive : decrypt output size not equal to origin size" << endl;
        return false;
    }
    return true;
}

bool CryptoPrimitive::keyExchangeEncrypt(u_char* dataBuffer, const int dataSize, u_char* key, u_char* iv, u_char* ciphertext)
{
    int cipherlen, len;
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    if (EVP_EncryptInit_ex(cipherctx_, EVP_aes_256_cfb(), nullptr, key, iv) != 1) {
        cerr << "encrypt error\n";
        return false;
    }

    if (EVP_EncryptUpdate(cipherctx_, ciphertext, &cipherlen, dataBuffer, dataSize) != 1) {
        cerr << "encrypt error\n";
        return false;
    }

    if (EVP_EncryptFinal_ex(cipherctx_, ciphertext + cipherlen, &len) != 1) {
        cerr << "encrypt error\n";
        return false;
    }
    cipherlen += len;
    if (cipherlen != dataSize) {
        cerr << "CryptoPrimitive : encrypt output size not equal to origin size" << endl;
        return false;
    }
    return true;
}

bool CryptoPrimitive::cmac128(vector<string>& message, string& mac, u_char* key, int keyLen)
{
    if (CMAC_Init(cmacctx_, key, keyLen, EVP_aes_128_cbc(), nullptr) != 1) {
        cerr << "cmac error\n";
        return false;
    }

    for (auto it : message) {
        CMAC_Update(cmacctx_, (void*)&it[0], it.length());
    }

    mac.clear();
    mac.resize(16);
    size_t maclen;

    if (CMAC_Final(cmacctx_, (unsigned char*)&mac[0], &maclen) != 1) {
        cerr << "cmac error" << endl;
        return false;
    }

    mac.resize(maclen);
    return true;
}
