#include "km_enclave_t.h"
#include "mbusafecrt.h"
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <sgx_tae_service.h>
#include <sgx_tcrypto.h>
#include <sgx_tkey_exchange.h>
#include <sgx_utils.h>
#include <string.h>

#define MAX_SPECULATIVE_KEY_SIZE 5 * 4 * 1024 * 1024
#define MAX_SPECULATIVE_CLIENT_SIZE 1

// static const sgx_ec256_public_t def_service_public_key = {
//     { 0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
//         0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
//         0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
//         0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38 },
//     { 0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
//         0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
//         0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
//         0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06 }

// }; //little endding/hard coding

#define PSE_RETRIES 5 /* Arbitrary. Not too long, not too short. */
/*
 * quote pow_enclave
 * */

sgx_status_t enclave_ra_init(sgx_ec256_public_t key, int b_pse,
    sgx_ra_context_t* ctx, sgx_status_t* pse_status)
{
    sgx_status_t ra_status;

    /*
   * If we want platform services, we must create a PSE session
   * before calling sgx_ra_init()
   */

    if (b_pse) {
        int retries = PSE_RETRIES;
        do {
            *pse_status = sgx_create_pse_session();
            if (*pse_status != SGX_SUCCESS)
                return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if (*pse_status != SGX_SUCCESS)
            return SGX_ERROR_UNEXPECTED;
    }

    ra_status = sgx_ra_init(&key, b_pse, ctx);

    if (b_pse) {
        int retries = PSE_RETRIES;
        do {
            *pse_status = sgx_create_pse_session();
            if (*pse_status != SGX_SUCCESS)
                return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if (*pse_status != SGX_SUCCESS)
            return SGX_ERROR_UNEXPECTED;
    }

    return ra_status;
}

sgx_status_t enclave_ra_close(sgx_ra_context_t ctx)
{
    sgx_status_t ret;
    ret = sgx_ra_close(ctx);
    return ret;
}

int encrypt(uint8_t* plaint, uint32_t plaintLen, uint8_t* symKey,
    uint32_t symKeyLen, uint8_t* cipher, uint32_t* cipherLen);

int decrypt(uint8_t* cipher, uint32_t cipherLen, uint8_t* symKey,
    uint32_t symKeyLen, uint8_t* plaint, uint32_t* plaintLen);

sgx_ra_key_128_t sessionkey;
sgx_ra_key_128_t macKey;
uint8_t currentSessionKey[32];
uint8_t* serverSecret;
uint32_t keyRegressionMaxTimes_;
uint32_t keyRegressionCurrentTimes_;
int nextEncryptMaskSetupFlag_ = 0;
uint8_t* nextEncryptionMaskSet_;
uint8_t currentXORBase[32 * 3000];

sgx_status_t ecall_setServerSecret(uint8_t* keyd, uint32_t keydLen)
{
    serverSecret = (uint8_t*)malloc(32);
    uint8_t* secretTemp = (uint8_t*)malloc(64 + keydLen);
    memset(secretTemp, 1, keydLen + 64);
    memcpy_s(secretTemp + 64, 128, keyd, keydLen);
    sgx_status_t sha256Status = sgx_sha256_msg(secretTemp, 64 + keydLen,
        (sgx_sha256_hash_t*)serverSecret);
    free(secretTemp);
    return sha256Status;
}

sgx_status_t ecall_setKeyRegressionCounter(uint32_t keyRegressionMaxTimes)
{
    keyRegressionMaxTimes_ = keyRegressionMaxTimes;
    keyRegressionCurrentTimes_ = keyRegressionMaxTimes_;
    return SGX_SUCCESS;
}

sgx_status_t ecall_setSessionKey(sgx_ra_context_t* ctx)
{
    sgx_status_t ret_status;
    ret_status = sgx_ra_get_keys(*ctx, SGX_RA_KEY_SK, &sessionkey);
    if (ret_status != SGX_SUCCESS) {
        return ret_status;
    } else {
        ret_status = sgx_ra_get_keys(*ctx, SGX_RA_KEY_MK, &macKey);
        if (ret_status != SGX_SUCCESS) {
            return ret_status;
        } else {
            memcpy(currentSessionKey, &sessionkey, sizeof(sgx_ra_key_128_t));
            memcpy(currentSessionKey + sizeof(sgx_ra_key_128_t), &macKey, sizeof(sgx_ra_key_128_t));
            uint8_t* hashDataTemp = (uint8_t*)malloc(32);
            uint8_t* hashResultTemp = (uint8_t*)malloc(32);
            memcpy_s(hashDataTemp, 32, &currentSessionKey, 32);
            for (int i = 0; i < keyRegressionCurrentTimes_; i++) {
                sgx_status_t sha256Status = sgx_sha256_msg(hashDataTemp, 32, (sgx_sha256_hash_t*)hashResultTemp);
                if (sha256Status != SGX_SUCCESS) {
                    return -1;
                }
                memcpy_s(hashDataTemp, 32, hashResultTemp, 32);
            }
            memcpy_s(currentSessionKey, 32, hashResultTemp, 32);
            return SGX_SUCCESS;
        }
    }
}

sgx_status_t ecall_setNextEncryptionMask(int clientID, uint32_t previousCountNumber, uint8_t* nonce, uint32_t nonceLen)
{
    nextEncryptMaskSetupFlag_ = 0;
    EVP_CIPHER_CTX* cipherctx_ = EVP_CIPHER_CTX_new();
    unsigned char currentKeyBase[32];
    unsigned char currentKey[32];
    int cipherlen, len;
    uint32_t currentCounter = previousCountNumber;
    EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
    for (int i = 0; i < MAX_SPECULATIVE_KEY_SIZE / 32; i++) {
        memcpy(currentKeyBase, &currentCounter, sizeof(uint32_t));
        memcpy(currentKeyBase + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
        currentCounter++;
        memcpy(currentKeyBase + 16, &currentCounter, sizeof(uint32_t));
        memcpy(currentKeyBase + 16 + sizeof(uint32_t), nonce, 16 - sizeof(uint32_t));
        currentCounter++;
        if (EVP_EncryptInit_ex(cipherctx_, EVP_aes_256_ecb(), NULL, currentSessionKey, currentSessionKey) != 1) {
            EVP_CIPHER_CTX_cleanup(cipherctx_);
            return SGX_ERROR_UNEXPECTED;
        }

        if (EVP_EncryptUpdate(cipherctx_, currentKey, &cipherlen, currentKeyBase, 32) != 1) {
            EVP_CIPHER_CTX_cleanup(cipherctx_);
            return SGX_ERROR_UNEXPECTED;
        }

        if (EVP_EncryptFinal_ex(cipherctx_, currentKey + cipherlen, &len) != 1) {
            EVP_CIPHER_CTX_cleanup(cipherctx_);
            return SGX_ERROR_UNEXPECTED;
        }
        memcpy_s(nextEncryptionMaskSet_ + clientID * MAX_SPECULATIVE_KEY_SIZE + i * 32, MAX_SPECULATIVE_CLIENT_SIZE * MAX_SPECULATIVE_KEY_SIZE, currentKey, 32);
    }
    EVP_CIPHER_CTX_cleanup(cipherctx_);
    nextEncryptMaskSetupFlag_ = 1;
    return SGX_SUCCESS;
}

sgx_status_t ecall_setSessionKeyUpdate(sgx_ra_context_t* ctx)
{
    keyRegressionCurrentTimes_--;
    memset(currentSessionKey, 0, 32);
    memcpy(&currentSessionKey, &sessionkey, sizeof(sgx_ra_key_128_t));
    memcpy(&currentSessionKey + sizeof(sgx_ra_key_128_t), &macKey, sizeof(sgx_ra_key_128_t));
    uint8_t* hashDataTemp = (uint8_t*)malloc(32);
    uint8_t* hashResultTemp = (uint8_t*)malloc(32);
    memcpy_s(hashDataTemp, 32, &currentSessionKey, 32);
    for (int i = 0; i < keyRegressionCurrentTimes_; i++) {
        sgx_status_t sha256Status = sgx_sha256_msg(hashDataTemp, 32, (sgx_sha256_hash_t*)hashResultTemp);
        if (sha256Status != SGX_SUCCESS) {
            free(hashDataTemp);
            free(hashResultTemp);
            return 1;
        }
        memcpy_s(hashDataTemp, 32, hashResultTemp, 32);
    }
    memcpy_s(&currentSessionKey, 32, hashResultTemp, 32);
    free(hashDataTemp);
    free(hashResultTemp);
    return 2;
}

sgx_status_t ecall_getCurrentSessionKey(char* currentSessionKeyResult)
{
    memcpy(currentSessionKeyResult, currentSessionKey, 32);
    return SGX_SUCCESS;
}

sgx_status_t ecall_setCTRMode()
{
    nextEncryptionMaskSet_ = (uint8_t*)malloc(MAX_SPECULATIVE_CLIENT_SIZE * MAX_SPECULATIVE_KEY_SIZE * sizeof(uint8_t));
    if (nextEncryptionMaskSet_ == NULL) {
        return SGX_ERROR_UNEXPECTED;
    }
    return SGX_SUCCESS;
}

sgx_status_t ecall_keygen_ctr(uint8_t* src, uint32_t srcLen, uint8_t* key, int clientID, uint32_t previousCounter, uint32_t currentCounter, uint8_t* nonce, uint32_t nonceLen)
{
    uint8_t *originhash, *hashTemp, *keySeed, *hash;
    hash = (uint8_t*)malloc(32);
    originhash = (uint8_t*)malloc(srcLen);
    keySeed = (uint8_t*)malloc(srcLen);
    hashTemp = (uint8_t*)malloc(64);
    unsigned char mask[srcLen * 2];

    if ((16 * (currentCounter + (srcLen / 32) * 4)) < MAX_SPECULATIVE_KEY_SIZE && nextEncryptMaskSetupFlag_ == 1) {
        for (int i = 0; i < srcLen; i++) {
            originhash[i] = src[i] ^ nextEncryptionMaskSet_[clientID * MAX_SPECULATIVE_KEY_SIZE + currentCounter * 16 + i];
        }
    } else {
        EVP_CIPHER_CTX* cipherctx_ = EVP_CIPHER_CTX_new();
        if (cipherctx_ == NULL) {
            free(hash);
            free(originhash);
            free(hashTemp);
            free(keySeed);
            return SGX_ERROR_UNEXPECTED;
        }
        unsigned char currentKeyBase[32];
        unsigned char currentKey[32];
        int cipherlen, len;
        EVP_CIPHER_CTX_set_padding(cipherctx_, 0);
        uint32_t currentCounterTemp = previousCounter + currentCounter;
        for (int i = 0; i < srcLen * 2 / 32; i++) {
            memcpy_s(currentKeyBase, 32, &currentCounterTemp, sizeof(uint32_t));
            memcpy_s(currentKeyBase + sizeof(uint32_t), 32, nonce, 16 - sizeof(uint32_t));
            currentCounterTemp++;
            memcpy_s(currentKeyBase + 16, 32, &currentCounterTemp, sizeof(uint32_t));
            memcpy_s(currentKeyBase + 16 + sizeof(uint32_t), 32, nonce, 16 - sizeof(uint32_t));
            currentCounterTemp++;
            if (!EVP_EncryptInit_ex(cipherctx_, EVP_aes_256_ecb(), NULL, currentSessionKey, currentSessionKey)) {
                free(hash);
                free(originhash);
                free(hashTemp);
                free(keySeed);
                return SGX_ERROR_UNEXPECTED;
            }
            if (EVP_EncryptUpdate(cipherctx_, currentKey, &cipherlen, currentKeyBase, 32) != 1) {
                free(hash);
                free(originhash);
                free(hashTemp);
                free(keySeed);
                return SGX_ERROR_UNEXPECTED;
            }
            if (EVP_EncryptFinal_ex(cipherctx_, currentKey + cipherlen, &len) != 1) {
                free(hash);
                free(originhash);
                free(hashTemp);
                free(keySeed);
                return SGX_ERROR_UNEXPECTED;
            }
            memcpy_s(mask + i * 32, srcLen * 2, currentKey, 32);
        }
        EVP_CIPHER_CTX_cleanup(cipherctx_);
        for (int i = 0; i < srcLen; i++) {
            originhash[i] = src[i] ^ mask[i];
        }
    }
    for (uint32_t index = 0; index < (srcLen / 32); index++) {
        memcpy_s(hashTemp, 64, originhash + index * 32, 32);
        memcpy_s(hashTemp + 32, 64, serverSecret, 32);
        sgx_status_t sha256Status = sgx_sha256_msg(hashTemp, 64, (sgx_sha256_hash_t*)hash);
        if (sha256Status != SGX_SUCCESS) {
            free(hash);
            free(originhash);
            free(hashTemp);
            free(keySeed);
            return sha256Status;
        }
        memcpy_s(keySeed + index * 32, srcLen - index * 32, hash, 32);
    }
    if ((currentCounter * 16 + srcLen) < MAX_SPECULATIVE_KEY_SIZE && nextEncryptMaskSetupFlag_ == 1) {
        for (int i = 0; i < srcLen; i++) {
            key[i] = keySeed[i] ^ nextEncryptionMaskSet_[clientID * MAX_SPECULATIVE_KEY_SIZE + currentCounter * 16 + srcLen + i];
        }
    } else {
        for (int i = 0; i < srcLen; i++) {
            key[i] = keySeed[i] ^ mask[i + srcLen];
        }
    }
    free(hash);
    free(originhash);
    free(hashTemp);
    free(keySeed);
    return SGX_SUCCESS;
}

sgx_status_t ecall_keygen(uint8_t* src, uint32_t srcLen, uint8_t* key)
{
    uint8_t *originhash, *hashTemp, *keySeed, *hash;
    uint32_t decryptLen, encryptLen;
    hash = (uint8_t*)malloc(32);
    originhash = (uint8_t*)malloc(srcLen);
    keySeed = (uint8_t*)malloc(srcLen);
    hashTemp = (uint8_t*)malloc(64);

    if (!decrypt(src, srcLen, currentSessionKey, 32, originhash, &decryptLen)) {
        free(hash);
        free(originhash);
        free(hashTemp);
        free(keySeed);
        return 1;
    }
    for (uint32_t index = 0; index < (decryptLen / 32); index++) {
        memcpy_s(hashTemp, 64, originhash + index * 32, 32);
        memcpy_s(hashTemp + 32, 64, serverSecret, 32);
        sgx_status_t sha256Status = sgx_sha256_msg(hashTemp, 64, (sgx_sha256_hash_t*)hash);
        if (sha256Status != SGX_SUCCESS) {
            free(hash);
            free(originhash);
            free(hashTemp);
            free(keySeed);
            return 2;
        }
        memcpy_s(keySeed + index * 32, srcLen - index * 32, hash, 32);
    }

    if (!encrypt(keySeed, srcLen, currentSessionKey, 32, key, &encryptLen)) {
        free(hash);
        free(originhash);
        free(hashTemp);
        free(keySeed);
        return 3;
    }
    free(hash);
    free(originhash);
    free(hashTemp);
    free(keySeed);
    if (srcLen != decryptLen) {
        return 4;
    } else {
        return 5;
    }
}

int encrypt(uint8_t* plaint, uint32_t plaintLen, uint8_t* symKey,
    uint32_t symKeyLen, uint8_t* cipher, uint32_t* cipherLen)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        return 0;
    }
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, symKey, symKey)) {
        goto error;
    }

    if (!EVP_EncryptUpdate(ctx, cipher, (int*)cipherLen, plaint, plaintLen)) {
        goto error;
    }

    int len;
    if (!EVP_EncryptFinal_ex(ctx, cipher + *cipherLen, &len)) {
        goto error;
    }
    cipherLen += len;
    EVP_CIPHER_CTX_cleanup(ctx);
    return 1;
error:
    EVP_CIPHER_CTX_cleanup(ctx);
    return 0;
}

int decrypt(uint8_t* cipher, uint32_t cipherLen, uint8_t* symKey,
    uint32_t symKeyLen, uint8_t* plaint, uint32_t* plaintLen)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        return 0;
    }
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb(), NULL, symKey, symKey)) {
        goto error;
    }

    if (!EVP_DecryptUpdate(ctx, plaint, (int*)plaintLen, cipher, cipherLen)) {
        goto error;
    }

    int decryptLen;
    if (!EVP_DecryptFinal_ex(ctx, plaint + *plaintLen, &decryptLen)) {
        goto error;
    }
    plaintLen += decryptLen;

    EVP_CIPHER_CTX_cleanup(ctx);
    return 1;

error:
    EVP_CIPHER_CTX_cleanup(ctx);
    return 0;
}