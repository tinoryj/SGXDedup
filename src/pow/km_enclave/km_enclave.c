//
// Created by a on 12/14/18.
//


#include "enclave_t.h"
#include <string.h>
#include <sgx_utils.h>
#include <sgx_tae_service.h>
#include <sgx_tkey_exchange.h>
#include <sgx_tcrypto.h>
#include <openssl/bn.h>
#include <openssl/evp.h>


static const sgx_ec256_public_t def_service_public_key = {
        {
                0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
                0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
                0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
                0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
        },
        {
                0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
                0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
                0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
                0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
        }

};//little endding/hard coding

#define PSE_RETRIES	5	/* Arbitrary. Not too long, not too short. */

/*
 * quote pow_enclave
 * */

sgx_status_t enclave_ra_init(sgx_ec256_public_t key, int b_pse,
                             sgx_ra_context_t *ctx, sgx_status_t *pse_status) {
    sgx_status_t ra_status;

    /*
     * If we want platform services, we must create a PSE session
     * before calling sgx_ra_init()
     */

    if (b_pse) {
        int retries = PSE_RETRIES;
        do {
            *pse_status = sgx_create_pse_session();
            if (*pse_status != SGX_SUCCESS) return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if (*pse_status != SGX_SUCCESS) return SGX_ERROR_UNEXPECTED;
    }

    ra_status = sgx_ra_init(&key, b_pse, ctx);

    if (b_pse) {
        int retries = PSE_RETRIES;
        do {
            *pse_status = sgx_create_pse_session();
            if (*pse_status != SGX_SUCCESS) return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if (*pse_status != SGX_SUCCESS) return SGX_ERROR_UNEXPECTED;
    }

    return ra_status;
}

sgx_status_t enclave_ra_close(sgx_ra_context_t ctx) {
    sgx_status_t ret;
    ret = sgx_ra_close(ctx);
    return ret;
}

int encrypt(uint8_t* plaint,uint32_t plaintLen,
             uint8_t* symKey,uint32_t symKeyLen,
             uint8_t* cipher,uint32_t* cipherLen);

int decrypt(uint8_t* cipher,uint32_t cipherLen,
             uint8_t* symKey,uint32_t symKeyLen,
             uint8_t* plaint,uint32_t* plaintLen);

/*
 * work load km_enclave
 * */
sgx_status_t ecall_keygen(sgx_ra_context_t *ctx,
                          sgx_ra_key_type_t type,
                          uint8_t *src,
                          uint32_t srcLen,
                          uint8_t *keyd,
                          uint32_t keydLen,
                          uint8_t *keyn,
                          uint32_t keynLen,
                          uint8_t *key) {

    sgx_status_t ret_status;
    sgx_ra_key_128_t k;

    ret_status=sgx_ra_get_keys(*ctx,type,&k);
    if(ret_status != SGX_SUCCESS){
        return ret_status;
    }

    BIGNUM *d=BN_new(),*n=BN_new(),*result=BN_new();
    BN_bin2bn(keyd,keydLen,d);
    BN_bin2bn(keyn,keynLen,n);
    uint8_t *hash;
    uint32_t hashLen;
    hash=(unsigned char*)malloc(srcLen);
    if(hash==NULL)
        return SGX_ERROR_UNEXPECTED;

    BN_CTX* bnCtx=BN_CTX_new();
    if(bnCtx==NULL){
        free(hash);
        return SGX_ERROR_UNEXPECTED;
    }

    if(!decrypt(src,srcLen,k,16,hash,hashLen)){
        goto error;
    }
    BN_bin2bn(hash,srcLen,result);
    BN_mod_exp(result,result,d,n,bnCtx);
    BN_bn2bin(result,src+srcLen-BN_num_bits(result));
    if(!encrypt(src,16,k,16,key,16)){
        goto error;
    }
    BN_CTX_free(bnCtx);
    free(hash);
    return SGX_SUCCESS;

    error:
    free(hash);
    BN_CTX_free(bnCtx);
    return SGX_ERROR_UNEXPECTED;
}

int encrypt(uint8_t* plaint,uint32_t plaintLen,
             uint8_t* symKey,uint32_t symKeyLen,
             uint8_t* cipher,uint32_t* cipherLen) {
    uint8_t iv[16] = {0};
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL)
        return 0;

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_cfb(), NULL, symKey, iv)) {
        goto error;
    }

    if (!EVP_EncryptUpdate(ctx, cipher, cipherLen, plaint, plaintLen)) {
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

int decrypt(uint8_t* cipher,uint32_t cipherLen,
             uint8_t* symKey,uint32_t symKeyLen,
             uint8_t* plaint,uint32_t* plaintLen){

    uint8_t iv[16]={0};
    EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();
    if(ctx==NULL)
        return 0;

    if(!EVP_DecryptInit_ex(ctx,EVP_aes_128_cfb(),NULL,symKey,iv)){
        goto error;
    }

    if(!EVP_DecryptUpdate(ctx,plaint,plaintLen,cipher,cipherLen)){
        goto error;
    }

    int len;
    if(!EVP_DecryptFinal_ex(ctx,plaint+*plaintLen,&len)){
        goto error;
    }
    plaintLen+=len;

    EVP_CIPHER_CTX_cleanup(ctx);
    return 1;

    error:
    EVP_CIPHER_CTX_cleanup(ctx);
    return 0;
}
