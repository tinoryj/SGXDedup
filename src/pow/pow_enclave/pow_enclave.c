#include "pow_enclave_t.h"
#include "sgx_thread.h"
#include "sgx_trts.h"
#include "sgx_tseal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <sgx_tae_service.h>
#include <sgx_tcrypto.h>
#include <sgx_tkey_exchange.h>
#include <sgx_utils.h>

static const sgx_ec256_public_t def_service_public_key = {
    { 0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38 },
    { 0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06 }

}; //little endding/hard coding

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

sgx_ra_key_128_t k;
int setSessionKeyFlag = 0;
sgx_status_t ecall_setSessionKey(sgx_ra_context_t* ctx, sgx_ra_key_type_t type)
{
    sgx_status_t ret_status;
    ret_status = sgx_ra_get_keys(*ctx, type, &k);
    setSessionKeyFlag = 1;
    return ret_status;
}

/*
 * work load pow_enclave
 * */
sgx_status_t ecall_calcmac(
    uint8_t* src,
    uint32_t srcLen,
    uint8_t* cmac)
{
    sgx_sha256_hash_t chunkHash;
    sgx_cmac_state_handle_t cmac_ctx;
    sgx_status_t ret_status;
    if (setSessionKeyFlag != 1) {
        return SGX_ERROR_UNEXPECTED;
    }
    sgx_cmac128_init(&k, &cmac_ctx);

    int it, sz = 0;
    for (it = 0; it < srcLen; it = it + sz) {
        if (srcLen - sizeof(int) < it) {
            memset(cmac, 0, 16);
            return ret_status;
        }
        memcpy(&sz, &src[it], sizeof(int));
        it = it + 4;
        if (srcLen - it < sz) {
            memset(cmac, 0, 16);
            return ret_status;
        }
        ret_status = sgx_sha256_msg(&src[it], sz, &chunkHash);
        if (ret_status != SGX_SUCCESS) {
            return ret_status;
        }
        sgx_cmac128_update((uint8_t*)&chunkHash, 32, cmac_ctx);
    }

    sgx_cmac128_final(cmac_ctx, (sgx_cmac_128bit_tag_t*)cmac);
    sgx_cmac128_close(cmac_ctx);
    return ret_status;
}

sgx_status_t enclave_sealed_init(uint8_t* sealed_buf)
{
    if (sealed_buf == NULL) {
        return 1;
    }
    uint32_t len = sizeof(sgx_sealed_data_t) + sizeof(sgx_ra_key_128_t);
    uint32_t sgxCalSealedLen = sgx_calc_sealed_data_size(0, sizeof(sgx_ra_key_128_t));
    if (sgxCalSealedLen != len) {
        return 4;
    }
    uint32_t unsealed_data_length = 0;
    uint8_t* plain_text = NULL;
    uint32_t plain_text_length = 0;
    uint8_t* unsealed_data = (uint8_t*)malloc(sizeof(sgx_ra_key_128_t));
    uint8_t* sealed_data = (uint8_t*)malloc(sgxCalSealedLen);
    memcpy_s(sealed_data, sgxCalSealedLen, sealed_buf, sgxCalSealedLen);
    // Unseal current sealed buf
    int decryptSize = sgx_get_encrypt_txt_len((sgx_sealed_data_t*)sealed_data);
    sgx_status_t ret;
    ret = sgx_unseal_data((sgx_sealed_data_t*)sealed_data, plain_text, &plain_text_length, unsealed_data, &unsealed_data_length);
    memcpy_s(&k, sizeof(k), unsealed_data, decryptSize);
    setSessionKeyFlag = 1;
    if (ret == SGX_SUCCESS) {
        if (unsealed_data_length != sizeof(sgx_ra_key_128_t)) {
            return unsealed_data_length;
        } else {
            return 0;
        }
    } else {
        return 3;
    }
}

sgx_status_t enclave_sealed_close(uint8_t* sealed_buf)
{
    uint32_t sealed_len = sizeof(sgx_sealed_data_t) + sizeof(sgx_ra_key_128_t);
    uint32_t sgxCalSealedLen = sgx_calc_sealed_data_size(0, sizeof(sgx_ra_key_128_t));
    if (sgxCalSealedLen != sealed_len) {
        return 1;
    }
    // Check the sealed_buf length and check the outside pointers deeply
    if (sealed_buf == NULL) {
        return 2;
    }
    uint8_t* plain_text = NULL;
    uint8_t* target_data = (uint8_t*)malloc(sizeof(sgx_ra_key_128_t));
    uint32_t plain_text_length = 0;
    uint8_t* temp_sealed_buf = (uint8_t*)malloc(sealed_len);
    if (temp_sealed_buf == NULL) {
        return 4;
    }
    memset_s(temp_sealed_buf, sealed_len, 0, sealed_len);
    memcpy_s(target_data, sizeof(sgx_ra_key_128_t), &k, sizeof(sgx_ra_key_128_t));
    sgx_status_t ret = sgx_seal_data(plain_text_length, plain_text, sizeof(sgx_ra_key_128_t), target_data, sealed_len, (sgx_sealed_data_t*)temp_sealed_buf);
    if (ret != SGX_SUCCESS) {
        free(temp_sealed_buf);
        free(target_data);
        return 5;
    } else {
        // Backup the sealed data to outside buffer
        memcpy_s(sealed_buf, sealed_len, temp_sealed_buf, sealed_len);
        // uint8_t* unsealed_data = (uint8_t*)malloc(sizeof(sgx_ra_key_128_t));
        // uint32_t unsealed_data_length;
        // ret = sgx_unseal_data((sgx_sealed_data_t*)sealed_buf, plain_text, &plain_text_length, unsealed_data, &unsealed_data_length);
        free(temp_sealed_buf);
        free(target_data);
        // if (strcmp((const char*)unsealed_data, k) != 0) {
        //     return sgx_get_encrypt_txt_len((sgx_sealed_data_t*)temp_sealed_buf);
        // }
        return 0;
    }
}
