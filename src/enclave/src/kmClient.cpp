#include "kmClient.hpp"
#include "sgxErrorSupport.h"
// #include <sys/time.h>

using namespace std;
extern Configure config;

#if SYSTEM_BREAK_DOWN == 1
struct timeval timestartkmClient;
struct timeval timeendKmClient;
#endif

void PRINT_BYTE_ARRAY_KM(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

void print(const char* mem, uint32_t len, uint32_t type)
{
    if (type == 1) {
        cout << mem << endl;
    } else if (type == 3) {
        uint32_t number;
        memcpy(&number, mem, sizeof(uint32_t));
        cout << number << endl;
    } else if (type == 2) {
        if (!mem || !len) {
            fprintf(stderr, "\n( null )\n");
            return;
        }
        uint8_t* array = (uint8_t*)mem;
        fprintf(stderr, "%u bytes:\n{\n", len);
        uint32_t i = 0;
        for (i = 0; i < len - 1; i++) {
            fprintf(stderr, "0x%x, ", array[i]);
            if (i % 8 == 7)
                fprintf(stderr, "\n");
        }
        fprintf(stderr, "0x%x ", array[i]);
        fprintf(stderr, "\n}\n");
    }
}

#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
bool kmClient::maskGenerate()
{
    sgx_status_t retval;
    if (!enclave_trusted) {
        cerr << "KmClient : can't do mask generation before km_enclave trusted" << endl;
        return false;
    }
    sgx_status_t status;
    status = ecall_setNextEncryptionMask(_eid,
        &retval);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : ecall failed for mask generate, status = " << endl;
        sgxErrorReport(status);
        return false;
    }
    return true;
}

int kmClient::modifyClientStatus(int clientID, u_char* cipherBuffer, u_char* hmacBuffer)
{
    sgx_status_t retval;
    if (!enclave_trusted) {
        cerr << "KmClient : can't modify client status before km_enclave trusted" << endl;
        return -1;
    }
    sgx_status_t status;
    status = ecall_clientStatusModify(_eid,
        &retval,
        clientID,
        (uint8_t*)cipherBuffer,
        (uint8_t*)hmacBuffer);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : ecall failed for modify client list, status = " << endl;
        sgxErrorReport(status);
        return -1;
    } else {
        if (retval == SGX_ERROR_UNEXPECTED) {
            cerr << "KmClient : counter not correct, reset to 0" << endl;
            return CLIENT_COUNTER_REST; // reset counter
        } else if (retval == SGX_ERROR_INVALID_SIGNATURE) {
            cerr << "KmClient : client hmac not correct, require resend" << endl;
            return ERROR_RESEND; // resend message  (hmac not cpmpare)
        } else if (retval == SGX_SUCCESS) {
            cerr << "KmClient : init client info success" << endl;
            return SUCCESS; // success
        } else if (retval == SGX_ERROR_INVALID_PARAMETER) {
            cerr << "KmClient : nonce has been used, send regrenate message" << endl;
            return NONCE_HAS_USED;
        }
        return -1;
    }
}

bool kmClient::request(u_char* hash, int hashSize, u_char* key, int keySize, int clientID)
{
    sgx_status_t retval;
    if (!enclave_trusted) {
        cerr << "KmClient : can't do a request before km_enclave trusted" << endl;
        return false;
    }
    sgx_status_t status;
    uint8_t* ans = (uint8_t*)malloc(keySize);
    status = ecall_keygen_ctr(_eid,
        &retval,
        (uint8_t*)hash,
        (uint32_t)hashSize,
        ans,
        clientID);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : ecall failed for key generate, status = " << endl;
        sgxErrorReport(status);
        return false;
    } else if (retval == SGX_ERROR_INVALID_SIGNATURE) {
        cerr << "KmClient : client hash list hmac error, key generate failed" << endl;
        return false;
    }
    memcpy(key, ans, keySize);
    free(ans);
    return true;
}
#else
bool kmClient::request(u_char* hash, int hashSize, u_char* key, int keySize)
{
    sgx_status_t retval, status;
    if (!enclave_trusted) {
        cerr << "KmClient : can't do a request before pow_enclave trusted" << endl;
        return false;
    }
    uint8_t* ans = (uint8_t*)malloc(keySize);
    status = ecall_keygen(_eid,
        &retval,
        (uint8_t*)hash,
        (uint32_t)hashSize,
        ans);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : ecall failed for key generate, status = " << endl;
        sgxErrorReport(status);
        return false;
    } else if (retval == SGX_ERROR_INVALID_SIGNATURE) {
        cerr << "KmClient : client hash list hmac error, key generate failed" << endl;
        return false;
    }
    memcpy(key, ans, keySize);
    free(ans);
    return true;
}
#endif

kmClient::kmClient(string keyd, uint64_t keyRegressionMaxTimes)
{
    _keyd = keyd;
    keyRegressionMaxTimes_ = keyRegressionMaxTimes;
}

kmClient::~kmClient()
{
    sgx_status_t status;
    sgx_status_t retval;
    status = ecall_enclave_close(_eid, &retval);
    sgx_destroy_enclave(_eid);
}

bool kmClient::sessionKeyUpdate()
{
    sgx_status_t status, retval;
    status = ecall_setSessionKeyUpdate(_eid, &retval);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : session key regression ecall error, status = " << endl;
        sgxErrorReport(status);
        return false;
    } else {
#if SYSTEM_DEBUG_FLAG == 1
        char currentSessionKey[64];
        status = ecall_getCurrentSessionKey(_eid, &retval, currentSessionKey);
        cerr << "KmClient : Current session key = " << endl;
        PRINT_BYTE_ARRAY_KM(stdout, currentSessionKey, 32);
        cerr << "KmClient : Original session key = " << endl;
        PRINT_BYTE_ARRAY_KM(stdout, currentSessionKey + 32, 16);
        cerr << "KmClient : Original mac key = " << endl;
        PRINT_BYTE_ARRAY_KM(stdout, currentSessionKey + 48, 16);
#endif
        return true;
    }
}

bool kmClient::init(ssl* raSecurityChannel, SSL* sslConnection)
{
#if SYSTEM_BREAK_DOWN == 1
    long diff;
    double second;
#endif
    ra_ctx_ = 0xdeadbeef;
    raSecurityChannel_ = raSecurityChannel;
    sslConnection_ = sslConnection;
    sgx_status_t status;
    sgx_status_t retval;
    raclose(_eid, ra_ctx_);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartkmClient, NULL);
#endif
    status = ecall_enclave_close(_eid, &retval);
    sgx_destroy_enclave(_eid);
    enclave_trusted = doAttestation();
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendKmClient, NULL);
    diff = 1000000 * (timeendKmClient.tv_sec - timestartkmClient.tv_sec) + timeendKmClient.tv_usec - timestartkmClient.tv_usec;
    second = diff / 1000000.0;
    cout << "KmClient : remote attestation time = " << second << " s" << endl;
#endif
    if (enclave_trusted) {
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartkmClient, NULL);
#endif
        status = ecall_setServerSecret(_eid,
            &retval,
            (uint8_t*)_keyd.c_str(),
            (uint32_t)_keyd.length());
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendKmClient, NULL);
        diff = 1000000 * (timeendKmClient.tv_sec - timestartkmClient.tv_sec) + timeendKmClient.tv_usec - timestartkmClient.tv_usec;
        second = diff / 1000000.0;
        cout << "KmClient : set key enclave global secret time = " << second << " s" << endl;
#endif
        if (status == SGX_SUCCESS) {
#if SYSTEM_DEBUG_FLAG == 1
            uint8_t ans[32];
            status = ecall_getServerSecret(_eid, &retval, ans);
            cerr << "KmClient : current server secret = " << endl;
            PRINT_BYTE_ARRAY_KM(stderr, ans, 32);
#endif
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartkmClient, NULL);
#endif
            uint32_t keyRegressionCounter = config.getKeyRegressionMaxTimes();
            status = ecall_setKeyRegressionCounter(_eid,
                &retval,
                keyRegressionCounter);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendKmClient, NULL);
            diff = 1000000 * (timeendKmClient.tv_sec - timestartkmClient.tv_sec) + timeendKmClient.tv_usec - timestartkmClient.tv_usec;
            second = diff / 1000000.0;
            cout << "KmClient : set key regression max counter time = " << second << " s" << endl;
#endif
            if (status == SGX_SUCCESS) {
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartkmClient, NULL);
#endif
                status = ecall_setSessionKey(_eid,
                    &retval, &ra_ctx_);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendKmClient, NULL);
                diff = 1000000 * (timeendKmClient.tv_sec - timestartkmClient.tv_sec) + timeendKmClient.tv_usec - timestartkmClient.tv_usec;
                second = diff / 1000000.0;
                cout << "KmClient : set key enclave session key time = " << second << " s" << endl;
#endif
                if (status == SGX_SUCCESS) {
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartkmClient, NULL);
#endif
                    status = ecall_setCTRMode(_eid, &retval);
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendKmClient, NULL);
                    diff = 1000000 * (timeendKmClient.tv_sec - timestartkmClient.tv_sec) + timeendKmClient.tv_usec - timestartkmClient.tv_usec;
                    second = diff / 1000000.0;
                    cout << "KmClient : init enclave ctr mode time = " << second << " s" << endl;
#endif
                    if (status == SGX_SUCCESS) {
                        return true;
                    } else {
                        cerr << "KmClient : set key server offline mask generate space error, status = " << endl;
                        sgxErrorReport(status);
                        return false;
                    }
#else
                    return true;
#endif
                } else {
                    cerr << "KmClient : set key server generate regression session key error, status = " << endl;
                    sgxErrorReport(status);
                    return false;
                }
            } else {
                cerr << "KmClient : set key server key regression max counter error, status = " << endl;
                sgxErrorReport(status);
                return false;
            }
        } else {
            cerr << "KmClient : set key server secret error, status = " << endl;
            sgxErrorReport(status);
            return false;
        }
    } else {
        cerr << "KmClient : enclave not trusted by storage server" << endl;
        return false;
    }
}

void kmClient::raclose(sgx_enclave_id_t& eid, sgx_ra_context_t& ctx)
{
    sgx_status_t status;
    enclave_ra_close(eid, &status, ctx);
}

bool kmClient::doAttestation()
{
    sgx_status_t status, sgxrv, pse_status;
    sgx_msg01_t msg01;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t* msg2 = NULL;
    sgx_ra_msg3_t* msg3 = NULL;
    ra_msg4_t* msg4 = NULL;
    uint32_t msg0_extended_epid_group_id = 0;
    uint32_t msg3_sz;

    string enclaveName = config.getKMEnclaveName();
    cerr << "KmClient : start to create enclave" << endl;
    status = sgx_create_enclave(enclaveName.c_str(), SYSTEM_DEBUG_FLAG, &_token, &updated, &_eid, 0);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : Can not launch km_enclave : " << enclaveName << endl;
        sgxErrorReport(status);
        return false;
    }

    status = enclave_ra_init(_eid, &sgxrv, def_service_public_key, false,
        &ra_ctx_, &pse_status);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : pow_enclave ra init ecall failed, status =  " << endl;
        sgxErrorReport(status);
        return false;
    }
    if (sgxrv != SGX_SUCCESS) {
        cerr << "KmClient : pow_enclave ra init failed, status =  " << endl;
        sgxErrorReport(status);
        return false;
    }
    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        cerr << "KmClient : sgx ge epid failed, status = " << endl;
        sgxErrorReport(status);
        return false;
    }

    /* Generate msg1 */
    status = sgx_ra_get_msg1(ra_ctx_, _eid, sgx_ra_get_ga, &msg1);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        cerr << "KmClient : sgx error get msg1 ecall, status = " << endl;
        sgxErrorReport(status);
        return false;
    }
    msg01.msg0_extended_epid_group_id = msg0_extended_epid_group_id;
    char msg01Buffer[sizeof(sgx_msg01_t)];
    memcpy(msg01Buffer, &msg0_extended_epid_group_id, sizeof(msg0_extended_epid_group_id));
    memcpy(msg01Buffer + sizeof(msg0_extended_epid_group_id), &msg1, sizeof(sgx_ra_msg1_t));
    char msg2Buffer[SGX_MESSAGE_MAX_SIZE];
    int msg2RecvSize = 0;
    if (!raSecurityChannel_->send(sslConnection_, msg01Buffer, sizeof(msg01))) {
        cerr << "KmClient : msg01 send socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        return false;
    }
    if (!raSecurityChannel_->recv(sslConnection_, msg2Buffer, msg2RecvSize)) {
        cerr << "KmClient : msg2 recv socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        return false;
    }
    msg2 = (sgx_ra_msg2_t*)malloc(msg2RecvSize);
    memcpy(msg2, msg2Buffer, msg2RecvSize);
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "KmClient : Send msg01 and Recv msg2 success" << endl;
    fprintf(stderr, "msg2.g_b.gx      = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->g_b.gx, sizeof(msg2->g_b.gx));
    fprintf(stderr, "\nmsg2.g_b.gy      = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->g_b.gy, sizeof(msg2->g_b.gy));
    fprintf(stderr, "\nmsg2.spid        = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->spid, sizeof(msg2->spid));
    fprintf(stderr, "\nmsg2.quote_type  = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->quote_type, sizeof(msg2->quote_type));
    fprintf(stderr, "\nmsg2.kdf_id      = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->kdf_id, sizeof(msg2->kdf_id));
    fprintf(stderr, "\nmsg2.sign_ga_gb  = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->sign_gb_ga, sizeof(msg2->sign_gb_ga));
    fprintf(stderr, "\nmsg2.mac         = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->mac, sizeof(msg2->mac));
    fprintf(stderr, "\nmsg2.sig_rl_size = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->sig_rl_size, sizeof(msg2->sig_rl_size));
    fprintf(stderr, "\nmsg2.sig_rl      = ");
    PRINT_BYTE_ARRAY_KM(stderr, &msg2->sig_rl, msg2->sig_rl_size);
    fprintf(stderr, "\n");
    fprintf(stderr, "+++ msg2_size = %zu\n", sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size);
#endif
    /* Process Msg2, Get Msg3  */
    /* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

    status = sgx_ra_proc_msg2(ra_ctx_, _eid,
        sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
        sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
        &msg3, &msg3_sz);

    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        cerr << "KmClient : sgx_ra_proc_msg2 error, status = " << fixed << status << endl;
        sgxErrorReport(status);
        if (msg2 != nullptr) {
            free(msg2);
        }
        return false;
    } else {
        free(msg2);
    }

#if SYSTEM_DEBUG_FLAG == 1
    cout << "KmClient : process msg2 success" << endl;
#endif

    char msg3Buffer[msg3_sz];
    memcpy(msg3Buffer, msg3, msg3_sz);
    char msg4Buffer[SGX_MESSAGE_MAX_SIZE];
    int msg4RecvSize = 0;
    if (!raSecurityChannel_->send(sslConnection_, msg3Buffer, msg3_sz)) {
        cerr << "KmClient : msg3 send socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        return false;
    }

    if (!raSecurityChannel_->recv(sslConnection_, msg4Buffer, msg4RecvSize)) {
        cerr << "KmClient : msg4 recv socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        return false;
    }
    msg4 = (ra_msg4_t*)malloc(msg4RecvSize);
    memcpy(msg4, msg4Buffer, msg4RecvSize);
#if SYSTEM_DEBUG_FLAG == 1
    cout << "KmClient : send msg3 and Recv msg4 success" << endl;
#endif
    if (msg3 != nullptr) {
        free(msg3);
    }
    if (msg4->status) {
#if SYSTEM_DEBUG_FLAG == 1
        cout << "KmClient : Enclave TRUSTED" << endl;
#endif
    } else if (!msg4->status) {
        cerr << "KmClient : Enclave NOT TRUSTED" << endl;
        enclave_ra_close(_eid, &sgxrv, ra_ctx_);
        free(msg4);
        return false;
    }

    enclave_trusted = msg4->status;
    free(msg4);
    return true;
}