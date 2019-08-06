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
    mdctx_ = EVP_MD_CTX_new();
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
    EVP_MD_CTX_reset(mdctx_);
    EVP_CIPHER_CTX_reset(cipherctx_);
    EVP_MD_CTX_destroy(mdctx_);
    EVP_CIPHER_CTX_cleanup(cipherctx_);
    free(iv_);
    free(chunkKeyEncryptionKey_);
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
bool CryptoPrimitive::generateHash(u_char* dataBuffer, const int& dataSize, u_char* hash)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (ctx == nullptr) {
        cerr << "error initial MD ctx\n";
        return false;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        cerr << "hash error\n";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    if (EVP_DigestUpdate(ctx, dataBuffer, dataSize) != 1) {
        cerr << "hash error\n";
        EVP_MD_CTX_free(ctx);
        return false;
    }
    int hashSize;
    if (EVP_DigestFinal_ex(ctx, hash, (unsigned int*)&hashSize) != 1) {
        cerr << "hash error\n";
        EVP_MD_CTX_free(ctx);
        return false;
    }
    EVP_MD_CTX_free(ctx);
    return true;
}

/*
 * encrypt the data stored in a buffer with a key
 *
 * @param dataBuffer - the buffer that stores the data
 * @param dataSize - the size of the data
 * @param key - the key used to encrypt the data
 * @param ciphertext - the generated ciphertext <return>
 *
 * @return - a boolean value that indicates if the encryption succeeds
 */
bool CryptoPrimitive::encryptWithKey(u_char* dataBuffer, const int& dataSize, u_char* key, u_char* ciphertext)
{
    int ciphertextSize, ciphertextTailSize;

    EVP_EncryptInit_ex(cipherctx_, cipher_, NULL, key, iv_);
    /*disable padding to ensure that the generated ciphertext has the same size as the input data*/
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    EVP_EncryptUpdate(cipherctx_, ciphertext, &ciphertextSize, dataBuffer, dataSize);
    EVP_EncryptFinal_ex(cipherctx_, ciphertext + ciphertextSize, &ciphertextTailSize);

    ciphertextSize += ciphertextTailSize;

    if (ciphertextSize != dataSize) {
        cerr << "CryptoPrimitive Error: the size of the cipher output (" << ciphertextSize << " bytes) does not match with that of the input (" << dataSize << " bytes)!" << endl;
        return false;
    }
    return true;
}

/*
 * decrypt the data stored in a buffer with a key
 *
 * @param dataBuffer - the buffer that stores the data
 * @param dataSize - the size of the data
 * @param key - the key used to encrypt the data
 * @param ciphertext - the generated ciphertext <return>
 *
 * @return - a boolean value that indicates if the encryption succeeds
 */

bool CryptoPrimitive::decryptWithKey(u_char* ciphertext, const int& dataSize, u_char* key, u_char* dataBuffer)
{
    int plaintextSize, plaintextTailSize;

    EVP_DecryptInit_ex(cipherctx_, cipher_, NULL, key, iv_);
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    EVP_DecryptUpdate(cipherctx_, dataBuffer, &plaintextSize, ciphertext, dataSize);
    EVP_DecryptFinal_ex(cipherctx_, dataBuffer + plaintextSize, &plaintextTailSize);

    plaintextSize += plaintextTailSize;

    if (plaintextSize != dataSize) {
        cerr << "CryptoPrimitive Error: the size of the plaintext output (" << plaintextSize << "bytes) does not match with that of the original data (" << dataSize << " bytes)!" << endl;

        return false;
    }

    return true;
}

bool CryptoPrimitive::encryptChunk(Chunk_t& chunk)
{
    u_char* ciphertext = nullptr;
    u_char* cipherKey = nullptr;
    if (!encryptWithKey(chunk.logicData, chunk.logicDataSize, chunk.encryptKey, ciphertext)) {
        cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
        return false;
    } else {
        memcpy(chunk.logicData, ciphertext, chunk.logicDataSize);
        return true;
    }
    if (!encryptWithKey(chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE, chunkKeyEncryptionKey_, cipherKey)) {
        cerr << "CryptoPrimitive Error: encrypt chunk logic data error" << endl;
        return false;
    } else {
        memcpy(chunk.encryptKey, cipherKey, CHUNK_ENCRYPT_KEY_SIZE);
        return true;
    }
}

bool CryptoPrimitive::decryptChunk(Chunk_t& chunk)
{
    u_char* plaintData = nullptr;
    u_char* plaintKey = nullptr;
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

bool CryptoPrimitive::keyExchangeDecrypt(u_char* ciphertext, const int& dataSize, u_char* key, u_char* iv, u_char* dataBuffer)
{
    int plaintextSize, plaintextTailSize;

    EVP_EncryptInit_ex(cipherctx_, EVP_aes_128_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    EVP_DecryptUpdate(cipherctx_, dataBuffer, &plaintextSize, ciphertext, dataSize);
    EVP_DecryptFinal_ex(cipherctx_, dataBuffer + plaintextSize, &plaintextTailSize);

    plaintextSize += plaintextTailSize;

    if (plaintextSize != dataSize) {
        cerr << "CryptoPrimitive Error: the size of the plaintext output (" << plaintextSize << "bytes) does not match with that of the original data (" << dataSize << " bytes)!" << endl;

        return false;
    }

    return true;
}

bool CryptoPrimitive::keyExchangeEncrypt(u_char* dataBuffer, const int& dataSize, u_char* key, u_char* iv, u_char* ciphertext)
{
    int ciphertextSize, ciphertextTailSize;

    EVP_EncryptInit_ex(cipherctx_, EVP_aes_128_cbc(), NULL, key, iv);
    /*disable padding to ensure that the generated ciphertext has the same size as the input data*/
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    EVP_EncryptUpdate(cipherctx_, ciphertext, &ciphertextSize, dataBuffer, dataSize);
    EVP_EncryptFinal_ex(cipherctx_, ciphertext + ciphertextSize, &ciphertextTailSize);

    ciphertextSize += ciphertextTailSize;

    if (ciphertextSize != dataSize) {
        cerr << "CryptoPrimitive Error: the size of the cipher output (" << ciphertextSize << " bytes) does not match with that of the input (" << dataSize << " bytes)!" << endl;
        return false;
    }
    return true;
}

bool CryptoPrimitive::cmac128(vector<string>& message, string& mac, u_char* key, int keyLen)
{
    CMAC_CTX* ctx = CMAC_CTX_new();

    if (ctx == nullptr) {
        cerr << "error initial cmac ctx\n";
        return false;
    }

    if (CMAC_Init(ctx, key, keyLen, EVP_aes_128_cbc(), nullptr) != 1) {
        cerr << "cmac error\n";
        CMAC_CTX_cleanup(ctx);
        return false;
    }

    for (auto it : message) {
        CMAC_Update(ctx, (void*)&it[0], it.length());
    }

    mac.clear();
    mac.resize(16);
    size_t maclen;

    if (CMAC_Final(ctx, (unsigned char*)&mac[0], &maclen) != 1) {
        cerr << "cmac error" << endl;
        CMAC_CTX_cleanup(ctx);
        return false;
    }

    mac.resize(maclen);
    CMAC_CTX_cleanup(ctx);
    return true;
}
