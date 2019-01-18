//
// Created by a on 12/14/18.
//


#include "Enclave_t.h"
#include "CryptoPrimitive.hpp"
#include <string.h>
#include <sgx_utils.h>
#include <sgx_tae_service.h>
#include <sgx_tkey_exchange.h>
#include <sgx_tcrypto.h>

# define SetKey \
    RSA_set0_key(key,                                           \
                 BN_bin2bn(n, sizeof(n)-1, NULL),               \
                 BN_bin2bn(e, sizeof(e)-1, NULL),               \
                 BN_bin2bn(d, sizeof(d)-1, NULL));              \
    RSA_set0_factors(key,                                       \
                     BN_bin2bn(p, sizeof(p)-1, NULL),           \
                     BN_bin2bn(q, sizeof(q)-1, NULL));          \
    RSA_set0_crt_params(key,                                    \
                        BN_bin2bn(dmp1, sizeof(dmp1)-1, NULL),  \
                        BN_bin2bn(dmq1, sizeof(dmq1)-1, NULL),  \
                        BN_bin2bn(iqmp, sizeof(iqmp)-1, NULL)); \
    memcpy(c, ctext_ex, sizeof(ctext_ex) - 1);                  \
    return (sizeof(ctext_ex) - 1);

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

sgx_status_t enclave_ra_init(sgx_ec256_public_t key, int b_pse,
                             sgx_ra_context_t *ctx, sgx_status_t *pse_status)
{
    sgx_status_t ra_status;

    /*
     * If we want platform services, we must create a PSE session
     * before calling sgx_ra_init()
     */

    if ( b_pse ) {
        int retries= PSE_RETRIES;
        do {
            *pse_status= sgx_create_pse_session();
            if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
    }

    ra_status= sgx_ra_init(&key, b_pse, ctx);

    if ( b_pse ) {
        int retries= PSE_RETRIES;
        do {
            *pse_status= sgx_create_pse_session();
            if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
        } while (*pse_status == SGX_ERROR_BUSY && retries--);
        if ( *pse_status != SGX_SUCCESS ) return SGX_ERROR_UNEXPECTED;
    }

    return ra_status;
}

sgx_status_t enclave_ra_close(sgx_ra_context_t ctx)
{
    sgx_status_t ret;
    ret = sgx_ra_close(ctx);
    return ret;
}

sgx_status_t ecall_calHash(sgx_ra_context_t* ctx,char *logicData,int logicDataLenh,char **signature,int signatureLen){
    static CryptoPrimitive dataCrypto(SHA1_TYPE),hashCrypto(SHA1_TYPE);
    vector<string>hashList;
    string Hash="",tmpChunkLogicData,tmpHash;
    tmpHash.resize(128);
    int it=0,chunkLength;
    while(it<logicDataLen){
        memcpy((char*)&chunkLength,&logicData[it],sizeof(int));
        it+=sizeof(int);
        tmpChunkLogicData.resize(chunkLength);
        memcpy(&tmpChunkLogicData[0],logicData[it],chunkLength);
        it+=chunkLength;
        dataCrypto.generaHash(tmpChunkLogicData,tmpHash);
        hashList.push_back(tmpHash);
    }

    hashCrypto.generaHash(hashList,Hash);

    //sign
    sgx_status_t status;
    sgx_ra_key_128_t *sk;
    status=sgx_ra_get_keys(ctx,SGX_RA_KEY_SK,sk);
    if(status!=SGX_SUCCESS){


    }

    hashCrypto.encryptWithKey(Hash,sk,signature);
}