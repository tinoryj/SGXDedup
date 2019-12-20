#include "pow_enclave_t.h"
#include <sgx_tae_service.h>
#include <sgx_tcrypto.h>
#include <sgx_tkey_exchange.h>
#include <sgx_utils.h>
#include <string.h>

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

/*
 * work load pow_enclave
 * */
sgx_status_t ecall_calcmac(sgx_ra_context_t* ctx,
    sgx_ra_key_type_t type,
    uint8_t* src,
    uint32_t srcLen,
    uint8_t* cmac)
{
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

    memset(k, 0, sizeof(k));

    sgx_cmac128_close(cmac_ctx);

    return ret_status;
}

int initialize_enclave(struct sealed_buf_t* sealed_buf)
{
    // sealed_buf == NULL indicates it is the first time to initialize the enclave
    if (sealed_buf == NULL) {
        return 0;
    }
    // It is not the first time to initialize the enclave
    // Reinitialize the enclave to recover the secret data from the input backup sealed data.
    uint32_t len = sizeof(sgx_sealed_data_t) + sizeof(uint32_t);
    //Check the sealed_buf length and check the outside pointers deeply
    if (sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index)] == NULL || sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index + 1)] == NULL || !sgx_is_outside_enclave(sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index)], len) || !sgx_is_outside_enclave(sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index + 1)], len)) {
        print("Incorrect input parameter(s).\n");
        return -1;
    }
    // Retrieve the secret from current backup sealed data
    uint32_t unsealed_data = 0;
    uint32_t unsealed_data_length;
    uint8_t* plain_text = NULL;
    uint32_t plain_text_length = 0;
    uint8_t* temp_sealed_buf = (uint8_t*)malloc(len);
    if (temp_sealed_buf == NULL) {
        print("Out of memory.\n");
        return -1;
    }

    memcpy(temp_sealed_buf, sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index)], len);

    // Unseal current sealed buf
    sgx_status_t ret = sgx_unseal_data((sgx_sealed_data_t*)temp_sealed_buf, plain_text, &plain_text_length, (uint8_t*)&unsealed_data, &unsealed_data_length);
    if (ret == SGX_SUCCESS) {
        free_allocated_memory(temp_sealed_buf);
        return 0;
    } else {
        print("Failed to reinitialize the enclave.\n");
        free_allocated_memory(temp_sealed_buf);
        return -1;
    }
}

int increase_and_seal_data(struct sealed_buf_t* sealed_buf)
{
    uint32_t sealed_len = sizeof(sgx_sealed_data_t);
    // Check the sealed_buf length and check the outside pointers deeply
    if (sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index)] == NULL || sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index + 1)] == NULL || !sgx_is_outside_enclave(sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index)], sealed_len) || !sgx_is_outside_enclave(sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index + 1)], sealed_len)) {
        print("Incorrect input parameter(s).\n");
        return -1;
    }

    char string_buf[BUFSIZ] = { '\0' };
    uint32_t temp_secret = 0;
    uint8_t* plain_text = NULL;
    uint32_t plain_text_length = 0;
    uint8_t* temp_sealed_buf = (uint8_t*)malloc(sealed_len);
    if (temp_sealed_buf == NULL) {
        print("Out of memory.\n");
        return -1;
    }
    memset(temp_sealed_buf, 0, sealed_len);

    sgx_status_t ret = sgx_seal_data(plain_text_length, plain_text, sizeof(g_secret), (uint8_t*)&g_secret, sealed_len, (sgx_sealed_data_t*)temp_sealed_buf);
    if (ret != SGX_SUCCESS) {
        print("Failed to seal data\n");
        free_allocated_memory(temp_sealed_buf);
        return -1;
    }
    // Backup the sealed data to outside buffer
    memcpy(sealed_buf->sealed_buf_ptr[MOD2(sealed_buf->index + 1)], temp_sealed_buf, sealed_len);
    sealed_buf->index++;

    free_allocated_memory(temp_sealed_buf);

    return 0;
}
