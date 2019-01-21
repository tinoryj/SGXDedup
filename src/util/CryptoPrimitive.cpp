//
// Created by a on 11/17/18.
//

#include "CryptoPrimitive.hpp"


CryptoPrimitive::CryptoPrimitive(int cryptoType){
    ERR_load_crypto_strings();

    OpenSSL_add_all_algorithms();

    _mdCTX=EVP_MD_CTX_new();
    _cipherctx=EVP_CIPHER_CTX_new();
    _cryptoType = cryptoType;

    if (_cryptoType == HIGH_SEC_PAIR_TYPE) {
        EVP_MD_CTX_init(_mdCTX);
        _md = EVP_sha256();
        _hashSize = 32;
        EVP_CIPHER_CTX_init(_cipherctx);

        _cipher = EVP_aes_256_cbc();
        _keySize = 32;
        _blockSize = 16;

    }

    if (_cryptoType == LOW_SEC_PAIR_TYPE) {
        EVP_MD_CTX_init(_mdCTX);

        _md = EVP_md5();
        _hashSize = 16;

        EVP_CIPHER_CTX_init(_cipherctx);

        _cipher = EVP_aes_128_cbc();
        _keySize = 16;
        _blockSize = 16;

        _iv = (unsigned char *) malloc(sizeof(unsigned char) * _blockSize);
        memset(_iv, 0, _blockSize);
    }

    if (_cryptoType == SHA256_TYPE) {
        EVP_MD_CTX_init(_mdCTX);

        _md = EVP_sha256();
        _hashSize = 32;

        _keySize = -1;
        _blockSize = -1;
    }

    if (_cryptoType == SHA1_TYPE) {
        EVP_MD_CTX_init(_mdCTX);
        _md = EVP_sha1();
        _hashSize = 20;

        _keySize = -1;
        _blockSize = -1;

    }
}
CryptoPrimitive::~CryptoPrimitive(){
    if ((_cryptoType == HIGH_SEC_PAIR_TYPE) || (_cryptoType == LOW_SEC_PAIR_TYPE)) {
        EVP_MD_CTX_free(_mdCTX);

        EVP_CIPHER_CTX_cleanup(_cipherctx);
        delete _iv;
    }

    if ((_cryptoType == SHA256_TYPE) || (_cryptoType == SHA1_TYPE)) {
        EVP_MD_CTX_free(_mdCTX);
    }


    EVP_cleanup();

    CRYPTO_cleanup_all_ex_data();

    ERR_free_strings();
}



bool CryptoPrimitive::generaHash(vector<string>data,string &hash){
    int hashSize;

    EVP_DigestInit_ex(_mdCTX, _md, nullptr);

    for(auto it:data) {
        EVP_DigestUpdate(_mdCTX, it.c_str(), it.length());
    }

    hash.resize(_hashSize);
    EVP_DigestFinal_ex(_mdCTX,(unsigned char*)&hash[0], (unsigned int*) &hashSize);

    if (hashSize != _hashSize) {
        /*fprintf(stderr, "Error: the size of the generated hash (%d bytes) does not match with the expected one (%d bytes)!\n",
                hashSize, _hashSize);*/

        return 0;
    }
    return 1;
}

bool CryptoPrimitive::generaHash(string data,string &hash){
    int hashSize;

    EVP_DigestInit_ex(_mdCTX, _md, nullptr);

    EVP_DigestUpdate(_mdCTX, data.c_str(), data.length());

    hash.resize(_hashSize);
    EVP_DigestFinal_ex(_mdCTX,(unsigned char*)&hash[0], (unsigned int*) &hashSize);

    if (hashSize != _hashSize) {
        /*fprintf(stderr, "Error: the size of the generated hash (%d bytes) does not match with the expected one (%d bytes)!\n",
                hashSize, _hashSize);*/

        return 0;
    }
    return 1;
}


bool CryptoPrimitive::encryptWithKey(string data,string key,string& cipherText) {
    int cipherTextSize, cipherTextTailSize;

    /**********************************/
    char* buffer=new char[4096];
    /**********************************/

    if (data.length() % _blockSize != 0) {
        std::cerr<< "Error: the size of the input data ("<<data.length()<<" bytes) is not a multiple of that of encryption block unit ( "<<_blockSize<<" bytes)!\n";
        return 0;
    }

    EVP_EncryptInit_ex(_cipherctx, _cipher, nullptr, (unsigned char*)key.c_str(), _iv);
    EVP_CIPHER_CTX_set_padding(_cipherctx, 0);
    EVP_EncryptUpdate(_cipherctx, (unsigned char*)buffer, &cipherTextSize, (unsigned char*)data.c_str(), data.length());
    EVP_EncryptFinal_ex(_cipherctx, (unsigned char*)buffer + cipherTextSize, &cipherTextTailSize);
    cipherTextSize += cipherTextTailSize;



    if (cipherTextSize != data.length()) {
        /*fprintf(stderr, "Error: the size of the cipher output (%d bytes) does not match with that of the input (%d bytes)!\n",
                cipherTextSize, data.length());*/

        return 0;
    }

    cipherText=buffer;

    return 1;
}

bool CryptoPrimitive::decryptWithKey(string cipherText,string key,string &plaintText){
    int plaintTextSize, plaintTextTailSize;

    /******************************/
    char *buffer=new char[4096];
    /******************************/

    if (cipherText.length() % _blockSize != 0) {
        /*fprintf(stderr, "Error: the size of the input data (%d bytes) is not a multiple of that of encryption block unit (%d bytes)!\n",
                cipherText.length(), _blockSize);
                */

        return 0;
    }

    EVP_DecryptInit_ex(_cipherctx, _cipher, nullptr, (unsigned char*)key.c_str(), _iv);
    EVP_CIPHER_CTX_set_padding(_cipherctx, 0);
    EVP_DecryptUpdate(_cipherctx, (unsigned char*)buffer, &plaintTextSize, (unsigned char*)cipherText.c_str(), cipherText.length());
    EVP_DecryptFinal_ex(_cipherctx,(unsigned char*) buffer + plaintTextSize, &plaintTextTailSize);
    plaintTextSize += plaintTextTailSize;

    if (plaintTextSize != cipherText.length()) {
        /*
        fprintf(stderr, "Error: the size of the plaintText output (%d bytes) does not match with that of the original data (%d bytes)!\n",
                plaintTextSize, cipherText.length());
        */
        return 0;
    }

    plaintText=buffer;
    return 1;
}


int CryptoPrimitive::getHashSize() {
    return _hashSize;
}

int CryptoPrimitive::getKeySize() {
    return _keySize;
}

int CryptoPrimitive::getBlockSize() {
    return _blockSize;
}


int CryptoPrimitive::cmac128(unsigned char *key, unsigned char *message, size_t mlen, unsigned char *mac) {
    size_t maclen;


    CMAC_CTX *ctx= CMAC_CTX_new();
    if ( ctx == nullptr ) {
        return 0;
    }

    if ( ! CMAC_Init(ctx, key, 16, EVP_aes_128_cbc(), nullptr) ) {
        CMAC_CTX_free(ctx);
        return 0;
    }

    if ( ! CMAC_Update(ctx, message, mlen) ) {
        CMAC_CTX_free(ctx);
        return 0;
    }

    if ( ! CMAC_Final(ctx, mac, &maclen) ) {
        CMAC_CTX_free(ctx);
        return 0;
    }

    return 1;
}


EVP_PKEY* CryptoPrimitive::key_generate() {
    EVP_PKEY *key= nullptr;
    EVP_PKEY_CTX *pctx= nullptr;
    EVP_PKEY_CTX *kctx= nullptr;
    EVP_PKEY *params= nullptr;


    /* Set up the parameter context */
    pctx= EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if ( pctx == nullptr ) {
        return nullptr;
    }

    /* Generate parameters for the P-256 curve */

    if ( ! EVP_PKEY_paramgen_init(pctx) ) {
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    if ( ! EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) ) {
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    if ( ! EVP_PKEY_paramgen(pctx, &params) ) {
        if ( params != nullptr ) EVP_PKEY_free(params);
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    /* Generate the key */

    kctx= EVP_PKEY_CTX_new(params, nullptr);
    if ( kctx == nullptr ) {
        EVP_PKEY_free(params);
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    if ( ! EVP_PKEY_keygen_init(kctx) ) {
        EVP_PKEY_CTX_free(kctx);
        EVP_PKEY_free(params);
        EVP_PKEY_CTX_free(pctx);
        return nullptr;
    }

    if ( ! EVP_PKEY_keygen(kctx, &key) ) {
        EVP_PKEY_CTX_free(kctx);
        EVP_PKEY_free(params);
        EVP_PKEY_CTX_free(pctx);
        EVP_PKEY_free(key);
        key= nullptr;
    }

    return key;
}

int CryptoPrimitive::key_to_sgx_ec256(sgx_ec256_public_t *k, EVP_PKEY *key) {
    EC_KEY *eckey= nullptr;
    const EC_POINT *ecpt= nullptr;
    EC_GROUP *ecgroup= nullptr;
    BIGNUM *gx= nullptr;
    BIGNUM *gy= nullptr;


    eckey= EVP_PKEY_get1_EC_KEY(key);
    if ( eckey == nullptr ) {
        goto cleanup;
    }

    ecgroup= EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    if ( ecgroup == nullptr ) {
        goto cleanup;
    }

    ecpt= EC_KEY_get0_public_key(eckey);

    gx= BN_new();
    if ( gx == nullptr ) {
        goto cleanup;
    }

    gy= BN_new();
    if ( gy == nullptr ) {
        goto cleanup;
    }

    if ( ! EC_POINT_get_affine_coordinates_GFp(ecgroup, ecpt, gx, gy, nullptr) ) {
        goto cleanup;
    }

    if ( ! BN_bn2lebinpad(gx, k->gx, sizeof(k->gx)) ) {
        goto cleanup;
    }

    if ( ! BN_bn2lebinpad(gy, k->gy, sizeof(k->gy)) ) {
        goto cleanup;
    }

    cleanup:
    if ( gy != nullptr ) BN_free(gy);
    if ( gx != nullptr ) BN_free(gx);
    if ( ecgroup != nullptr ) EC_GROUP_free(ecgroup);
    return 1;
}


int CryptoPrimitive::ecdsa_sign(unsigned char *msg, size_t mlen, EVP_PKEY *key, unsigned char *r, unsigned char *s,
                                unsigned char *digest) {
    ECDSA_SIG *sig = nullptr;
    EC_KEY *eckey = nullptr;
    const BIGNUM *bnr= nullptr;
    const BIGNUM *bns= nullptr;


    eckey= EVP_PKEY_get1_EC_KEY(key);
    if ( eckey == nullptr ) {
        return 0;
    }

    /* In ECDSA signing, we sign the sha256 digest of the message */

    if ( ! sha256_digest(msg, mlen, digest) ) {
        EC_KEY_free(eckey);
        return 0;
    }

    sig= ECDSA_do_sign(digest, 32, eckey);
    if ( sig == nullptr ) {
        EC_KEY_free(eckey);
        return 0;
    }



    ECDSA_SIG_get0(sig, &bnr, &bns);

    if ( ! BN_bn2binpad(bnr, r, 32) ) {
        ECDSA_SIG_free(sig);
        EC_KEY_free(eckey);
        return 0;
    }

    if ( ! BN_bn2binpad(bns, s, 32) ) {
        ECDSA_SIG_free(sig);
        EC_KEY_free(eckey);
        return 0;
    }

    return 1;
}

void CryptoPrimitive::reverse_bytes(void *dest, void *src, size_t n) {
    size_t i;
    char *sp= (char *)src;

    if ( n < 2 ) return;

    if ( src == dest ) {
        size_t j;

        for (i= 0, j= n-1; i<j; ++i, --j) {
            char t= sp[j];
            sp[j]= sp[i];
            sp[i]= t;
        }
    } else {
        char *dp= (char *) dest + n - 1;
        for (i= 0; i< n; ++i, ++sp, --dp) *dp= *sp;
    }
}


char* CryptoPrimitive::base64_encode(const char *msg, size_t sz) {
    BIO *b64, *bmem;
    char *bstr, *dup;
    int len;

    b64= BIO_new(BIO_f_base64());
    bmem= BIO_new(BIO_s_mem());

    /* Single line output, please */
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO_push(b64, bmem);

    if ( BIO_write(b64, msg, (int) sz) == -1 ) return nullptr;

    BIO_flush(b64);

    len= BIO_get_mem_data(bmem, &bstr);
    dup= (char *) malloc(len+1);
    memcpy(dup, bstr, len);
    dup[len]= 0;

    BIO_free(bmem);
    BIO_free(b64);

    return dup;
}

int CryptoPrimitive::sha256_digest(const unsigned char *msg, size_t mlen, unsigned char *digest) {
    EVP_MD_CTX *ctx;

    memset(digest, 0, 32);

    ctx= EVP_MD_CTX_new();
    if ( ctx == NULL ) {
        return 0;
    }

    if ( EVP_DigestInit(ctx, EVP_sha256()) != 1 ) {
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    if ( EVP_DigestUpdate(ctx, msg, mlen) != 1 ) {
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    if ( EVP_DigestFinal(ctx, digest, NULL) != 1 ) {
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    EVP_MD_CTX_free(ctx);
    return 0;
}


EVP_PKEY* CryptoPrimitive::key_from_sgx_ec256(sgx_ec256_public_t *k) {
    EC_KEY *key= NULL;
    EVP_PKEY *pkey= NULL;

    BIGNUM *gx= NULL;
    BIGNUM *gy= NULL;

    /* Get gx and gy as BIGNUMs */

    if ( (gx= BN_lebin2bn((unsigned char *) k->gx, sizeof(k->gx), NULL)) == nullptr ) {
        return nullptr;
    }

    if ( (gy= BN_lebin2bn((unsigned char *) k->gy, sizeof(k->gy), NULL)) == nullptr ) {
        BN_free(gx);
        return nullptr;
    }

    key= EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if ( key == NULL ) {
        BN_free(gy);
        BN_free(gx);
        return nullptr;
    }

    if ( ! EC_KEY_set_public_key_affine_coordinates(key, gx, gy) ) {
        EC_KEY_free(key);
        key= NULL;
        BN_free(gy);
        BN_free(gx);
        return nullptr;
    }

    /* Get the peer key as an EVP_PKEY object */

    pkey= EVP_PKEY_new();
    if ( pkey == NULL ) {
        BN_free(gy);
        BN_free(gx);
        return nullptr;
    }

    if ( ! EVP_PKEY_set1_EC_KEY(pkey, key) ) {
        EVP_PKEY_free(pkey);
        pkey= NULL;
        BN_free(gy);
        BN_free(gx);
        return nullptr;
    }

    return pkey;
}

unsigned char* CryptoPrimitive::key_shared_secret(EVP_PKEY *key, EVP_PKEY *peerkey, size_t *slen) {
    EVP_PKEY_CTX *sctx= NULL;
    unsigned char *secret= NULL;

    *slen= 0;

    /* Set up the shared secret derivation */

    sctx= EVP_PKEY_CTX_new(key, NULL);
    if ( sctx == NULL ) {;
        return nullptr;
    }

    if ( ! EVP_PKEY_derive_init(sctx) ) {
        EVP_PKEY_CTX_free(sctx);
        return nullptr;
    }

    if ( ! EVP_PKEY_derive_set_peer(sctx, peerkey) ) {
        EVP_PKEY_CTX_free(sctx);
        return nullptr;
    }

    /* Get the secret length */

    if ( ! EVP_PKEY_derive(sctx, NULL, slen) ) {
        EVP_PKEY_CTX_free(sctx);
        return nullptr;
    }

    secret=(unsigned char*) OPENSSL_malloc(*slen);
    if ( secret == NULL ) {
        EVP_PKEY_CTX_free(sctx);
        return nullptr;
    }

    /* Derive the shared secret */

    if ( ! EVP_PKEY_derive(sctx, secret, slen) ) {
        OPENSSL_free(secret);
        EVP_PKEY_CTX_free(sctx);
        secret= NULL;
    }


    return secret;
}