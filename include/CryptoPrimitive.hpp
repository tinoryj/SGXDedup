//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
#define GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <vector>

/*for the use of OpenSSL*/
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>

//#define OPENSSL_THREAD_DEFINES

#include <openssl/opensslconf.h>
/*macro for OpenSSL debug*/
#define OPENSSL_DEBUG 0
/*for the use of mutex lock*/
//#include <pthread.h>

/*macro for the type of a high secure pair of hash generation and encryption*/
#define HIGH_SEC_PAIR_TYPE 0
/*macro for the type of a low secure pair of hash generation and encryption*/
#define LOW_SEC_PAIR_TYPE 1
/*macro for the type of a SHA-256 hash generation*/
#define SHA256_TYPE 2
/*macro for the type of a SHA-1 hash generation*/
#define SHA1_TYPE 3

using namespace std;

class CryptoPrimitive{
private:

    int _cryptoType;

    EVP_MD_CTX _mdCTX;
    const EVP_MD *_md;

    EVP_CIPHER_CTX _cipherctx;
    const EVP_CIPHER *_cipher;

    unsigned char* _iv;

    int _hashSize;
    int _keySize;
    int _blockSize;

public:
    CryptoPrimitive(int cryptoType);
    ~CryptoPrimitive();
    int getHashSize();
    int getKeySize();
    int getBlockSize();

    bool generaHash(vector<string>data,string& hash);
    bool generaHash(string data,string& hash);
    bool encryptWithKey(string plaintext, string key, string& ciphertext);
    bool decryptWithKey(string ciphertext, string key, string& plaintext);

    //for pow

    int cmac128(unsigned char key[16], unsigned char *message, size_t mlen,
                unsigned char mac[16]);

    EVP_PKEY *key_generate();

    int key_to_sgx_ec256 (sgx_ec256_public_t *k, EVP_PKEY *key);

    int ecdsa_sign(unsigned char *msg, size_t mlen, EVP_PKEY *key,
                   unsigned char r[32], unsigned char s[32], unsigned char digest[32]);

    void reverse_bytes(void *dest, void *src, size_t n);

};


#endif //GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
