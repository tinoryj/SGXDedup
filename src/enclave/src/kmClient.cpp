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

kmClient::kmClient(string keyd)
{
    _keyd = keyd;
    init();
}

kmClient::~kmClient()
{
    sgx_status_t status;
    sgx_status_t retval;
    status = ecall_enclave_close(_eid, &retval);
    sgx_destroy_enclave(_eid);
}

bool kmClient::init()
{
#if SYSTEM_BREAK_DOWN == 1
    long diff;
    double second;
#endif
    ra_ctx_ = 0xdeadbeef;
    sgx_status_t status;
    sgx_status_t retval;
    raclose(_eid, ra_ctx_);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartkmClient, NULL);
#endif
    enclave_trusted = doAttestation();
    status = ecall_setServerSecret(_eid,
        &retval,
        (uint8_t*)_keyd.c_str(),
        (uint32_t)_keyd.length());

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
        status = ecall_setSessionKey(_eid, &retval, &ra_ctx_);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendKmClient, NULL);
        diff = 1000000 * (timeendKmClient.tv_sec - timestartkmClient.tv_sec) + timeendKmClient.tv_usec - timestartkmClient.tv_usec;
        second = diff / 1000000.0;
        cout << "KmClient : set key enclave session key time = " << setprecision(8) << second << " s" << endl;
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
            cout << "KmClient : init enclave ctr mode time = " << setprecision(8) << second << " s" << endl;
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
}

void kmClient::raclose(sgx_enclave_id_t& eid, sgx_ra_context_t& ctx)
{
    sgx_status_t status;
    enclave_ra_close(eid, &status, ctx);
}

bool kmClient::doAttestation()
{
    sgx_status_t status, sgxrv, pse_status;

    string enclaveName = config.getKMEnclaveName();
#if SYSTEM_DEBUG_FLAG == 1
    cerr << "KmClient : start to create enclave" << endl;
#endif
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, NULL, NULL, &_eid, NULL);
    if (status != SGX_SUCCESS) {
        cerr << "KmClient : Can not launch km_enclave : " << enclaveName << endl;
        sgxErrorReport(status);
        return false;
    }
    return true;
}