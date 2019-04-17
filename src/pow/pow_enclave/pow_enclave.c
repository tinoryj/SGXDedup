//
// Created by a on 12/14/18.
//


#include "enclave_t.h"
#include <string.h>
#include <sgx_utils.h>
#include <sgx_tae_service.h>
#include <sgx_tkey_exchange.h>
#include <sgx_tcrypto.h>

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


/*
 * work load pow_enclave
 * */
sgx_status_t ecall_calcmac(sgx_ra_context_t *ctx,
                           sgx_ra_key_type_t type,
                           uint8_t *src,
                           uint32_t srcLen,
                           uint8_t *cmac) {
    sgx_ra_key_128_t k;
    sgx_sha256_hash_t chunkHash;
    sgx_cmac_state_handle_t cmac_ctx;

    sgx_status_t ret_status;

    ret_status = sgx_ra_get_keys(*ctx, type, &k);
    if (ret_status != SGX_SUCCESS) {
        return ret_status;
    }
    sgx_cmac128_init(&k, &cmac_ctx);

    int it, sz = 0;
    for (it = 0; it < srcLen; it = it + sz) {
        if (srcLen - sizeof(int) < it) {
            memset(cmac, 0, sizeof(cmac));
            return ret_status;
        }
        memcpy(&sz, &src[it], sizeof(int));
        it = it + 4;
        if (srcLen - it < sz) {
            memset(cmac, 0, sizeof(cmac));
            return ret_status;
        }
        ret_status = sgx_sha256_msg(&src[it], sz, &chunkHash);
        if (ret_status != SGX_SUCCESS) {
            return ret_status;
        }
        sgx_cmac128_update((uint8_t *) &chunkHash, 32, cmac_ctx);
    }

    sgx_cmac128_final(cmac_ctx, (sgx_cmac_128bit_tag_t *) cmac);

    memset(k, 0, sizeof(k));

    sgx_cmac128_close(cmac_ctx);

    return ret_status;
}