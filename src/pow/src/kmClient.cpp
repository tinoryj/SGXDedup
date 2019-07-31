#include "kmClient.hpp"

using namespace std;
extern Configure config;

bool kmClient::request(u_char* hash, int hashSize, u_char* key, int keySize)
{
    sgx_status_t retval;
    if (!enclave_trusted) {
        cerr << "kmClient : can't do a request before pow_enclave trusted" << endl;
        return false;
    }

    sgx_status_t status;
    uint8_t* src = new uint8_t[hashSize];

    if (src == nullptr) {
        cerr << "kmClient: mem error" << endl;
        return false;
    }
    memcpy(src, hash, hashSize);
    uint8_t* ans = new uint8_t[1024];
    status = ecall_keygen(_eid,
        &retval,
        &_ctx,
        SGX_RA_KEY_SK,
        (uint8_t*)hash,
        (uint32_t)hashSize,
        (uint8_t*)_keyd.c_str(),
        (uint32_t)_keyd.length(),
        (uint8_t*)_keyn.c_str(),
        (uint32_t)_keyn.length(),
        ans);
    if (status != SGX_SUCCESS) {
        cerr << "kmClient : ecall failed" << endl;
        return false;
    }
    memcpy(key, ans, keySize);
    return true;
}

kmClient::kmClient(string keyn, string keyd)
{
    _keyn = keyn;
    _keyd = keyd;
}

bool kmClient::init(Socket socket)
{
    _ctx = 0xdeadbeef;
    _socket = socket;
    enclave_trusted = doAttestation();
    return enclave_trusted;
}

bool kmClient::trusted()
{
    return enclave_trusted;
}

bool kmClient::createEnclave(sgx_enclave_id_t& eid,
    sgx_ra_context_t& ctx,
    string enclaveName)
{
    sgx_status_t status, pse_status;

    if (sgx_create_enclave(enclaveName.c_str(),
            SGX_DEBUG_FLAG,
            &_token,
            &updated,
            &eid,
            0)
        != SGX_SUCCESS) {
        return false;
    }

    if (enclave_ra_init(eid,
            &status,
            def_service_public_key,
            false,
            &ctx,
            &pse_status)
        != SGX_SUCCESS) {
        return false;
    }

    if (!(status && pse_status)) {
        return false;
    }

    return true;
}

bool kmClient::getMsg01(sgx_enclave_id_t& eid,
    sgx_ra_context_t& ctx,
    string& msg01)
{
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

bool kmClient::processMsg2(sgx_enclave_id_t& eid,
    sgx_ra_context_t& ctx,
    string& Msg2,
    string& Msg3)
{
    sgx_ra_msg3_t* msg3;
    uint32_t msg3_sz;
    sgx_ra_msg2_t* msg2 = (sgx_ra_msg2_t*)new uint8_t[Msg2.length()];
    memcpy(msg2, &Msg2[0], Msg2.length());
    if (sgx_ra_proc_msg2(ctx,
            eid,
            sgx_ra_proc_msg2_trusted,
            sgx_ra_get_msg3_trusted,
            msg2,
            sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
            &msg3,
            &msg3_sz)
        != SGX_SUCCESS) {
        goto error;
    }

    Msg3.resize(msg3_sz);
    memcpy(&Msg3[0], msg3, msg3_sz);
    return true;

error:
    raclose(eid, ctx);
    return false;
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
    sgx_ra_msg2_t* msg2;
    sgx_ra_msg3_t* msg3;
    ra_msg4_t* msg4 = NULL;
    uint32_t msg0_extended_epid_group_id = 0;
    uint32_t msg3_sz;

    string enclaveName = config.getKMEnclaveName();
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, &_token, &updated, &_eid, 0);
    if (status != SGX_SUCCESS) {
        cerr << "kmClient : Can not launch km_enclave : " << enclaveName << endl;
        printf("%08x\n", status);
        return false;
    }

    status = enclave_ra_init(_eid, &sgxrv, def_service_public_key, false,
        &_ctx, &pse_status);
    if (status != SGX_SUCCESS) {
        cerr << "kmClient : pow_enclave ra init failed : " << status << endl;
        return false;
    }
    if (sgxrv != SGX_SUCCESS) {
        cerr << "kmClient : sgx ra init failed : " << sgxrv << endl;
        return false;
    }

    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "kmClient : sgx ge epid failed : " << status << endl;
        return false;
    }

    /* Generate msg1 */

    status = sgx_ra_get_msg1(_ctx, _eid, sgx_ra_get_ga, &msg01.msg1);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "kmClient : sgx error get msg1" << endl
             << status << endl;
        return false;
    }

    u_char msg01Buffer[sizeof(msg01)];
    memcpy(msg01Buffer, &msg01, sizeof(msg01));
    u_char msg2Buffer[SGX_MESSAGE_MAX_SIZE];
    int msg2RecvSize = 0;
    if (!_socket.Send(msg01Buffer, sizeof(msg01))) {
        cerr << "kmClient: socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }
    if (!_socket.Recv(msg2Buffer, msg2RecvSize)) {
        cerr << "kmClient: socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }
    msg2 = (sgx_ra_msg2_t*)new uint8_t[msg2RecvSize];
    memcpy(msg2, msg2Buffer, msg2RecvSize);
    cerr << "kmClient : Send msg01 and Recv msg2 success" << endl;

    /* Process Msg2, Get Msg3  */
    /* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

    status = sgx_ra_proc_msg2(_ctx, _eid,
        sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
        sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
        &msg3, &msg3_sz);

    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "kmClient : sgx_ra_proc_msg2 : " << status << endl;
        if (msg2 != nullptr) {
            //delete msg2;
        }
        return false;
    }
    cerr << "kmClient : process msg2 success" << endl;

    u_char msg3Buffer[msg3_sz];
    memcpy(msg3Buffer, msg3, msg3_sz);
    u_char msg4Buffer[SGX_MESSAGE_MAX_SIZE];
    int msg4RecvSize = 0;
    if (!_socket.Send(msg3Buffer, msg3_sz)) {
        cerr << "kmClient: socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }

    if (!_socket.Recv(msg4Buffer, msg4RecvSize)) {
        cerr << "kmClient: socket error" << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }
    msg4 = (ra_msg4_t*)new uint8_t[msg4RecvSize];
    memcpy(msg4, msg4Buffer, msg4RecvSize);
    cerr << "kmClient : send msg3 and Recv msg4 success" << endl;

    if (msg4->status) {
        cerr << "kmClient : Enclave TRUSTED" << endl;
    } else if (!msg4->status) {
        cerr << "kmClient : Enclave NOT TRUSTED" << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }

    enclave_trusted = msg4->status;

    //delete msg4;
    return true;
}