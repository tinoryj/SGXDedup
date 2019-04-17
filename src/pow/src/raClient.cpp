//
// Created by a on 3/20/19.
//

#include "raClient.hpp"

bool raClient::createEnclave(sgx_enclave_id_t &eid,
                             sgx_ra_context_t &ctx,
                             string enclaveName) {
    sgx_status_t status, pse_status;

    if (sgx_create_enclave(enclaveName.c_str(),
                           SGX_DEBUG_FLAG,
                           &_token,
                           &updated,
                           &eid,
                           0) != SGX_SUCCESS) {
        return false;
    }

    if (enclave_ra_init(eid,
                        &status,
                        def_service_public_key,
                        false,
                        &ctx,
                        &pse_status) != SGX_SUCCESS) {
        return false;
    }

    if (!(status && pse_status)) {
        return false;
    }

    return true;
}

bool raClient::getMsg01(sgx_enclave_id_t &eid,
                        sgx_ra_context_t &ctx,
                        string &msg01) {
    sgx_status_t status;
    uint32_t msg0;
    sgx_ra_msg1_t msg1;

    if (sgx_get_extended_epid_group_id(&msg0) != SGX_SUCCESS) {
        goto error;
    }

    if (sgx_ra_get_msg1(ctx, eid, sgx_ra_get_ga, &msg1) != SGX_SUCCESS) {
        goto error;
    }

    msg01.resize(sizeof msg0 + sizeof msg1);
    memcpy(&msg01[0], &msg1, sizeof msg1);
    memcpy(&msg01[sizeof msg1], &msg0, sizeof msg0);
    return true;

    error:
    raclose(eid, ctx);
    return false;
}

bool raClient::processMsg2(sgx_enclave_id_t &eid,
                           sgx_ra_context_t &ctx,
                           string &Msg2,
                           string &Msg3) {
    sgx_ra_msg3_t *msg3;
    uint32_t msg3_sz;
    sgx_ra_msg2_t *msg2 = (sgx_ra_msg2_t *) new uint8_t[Msg2.length()];
    memcpy(msg2, &Msg2[0], Msg2.length());
    if (sgx_ra_proc_msg2(ctx,
                         eid,
                         sgx_ra_proc_msg2_trusted,
                         sgx_ra_get_msg3_trusted,
                         msg2,
                         sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
                         &msg3,
                         &msg3_sz) != SGX_SUCCESS) {
        goto error;
    }

    Msg3.resize(msg3_sz);
    memcpy(&Msg3[0], msg3, msg3_sz);
    return true;

    error:
    raclose(eid, ctx);
    return false;
}

void raClient::raclose(sgx_enclave_id_t &eid,sgx_ra_context_t &ctx) {
    sgx_status_t status;
    enclave_ra_close(eid, &status, ctx);
}