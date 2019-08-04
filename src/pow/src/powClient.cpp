#include "../include/powClient.hpp"

using namespace std;

extern Configure config;

void PRINT_BYTE_ARRAY(
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

void powClient::run()
{
    vector<Data_t> batchChunk;
    vector<string> batchHash;
    uint64_t powBatchSize = config.getPOWBatchSize();
    u_char* batchChunkLogicData_charBuffer;
    batchChunkLogicData_charBuffer = (u_char*)malloc(sizeof(u_char) * (MAX_CHUNK_SIZE + sizeof(int)) * powBatchSize);
    powSignedHash_t request;
    RequiredChunk_t lists;
    Data_t tempChunk;
    int netstatus;

    while (true) {
        batchChunk.clear();
        batchHash.clear();
        request.hash_.clear();

        uint64_t currentBatchSize = 0;
        memset(batchChunkLogicData_charBuffer, 0, sizeof(u_char) * (MAX_CHUNK_SIZE + sizeof(int)) * powBatchSize);
        for (uint64_t i = 0; i < powBatchSize; i++) {
            if (inputMQ.done_ && !extractMQFromEncoder(tempChunk)) {
                senderObj->editJobDoneFlag();
                break;
            } else {
                if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                    insertMQToSender(tempChunk);
                    continue;
                }
                string tempChunkHash;
                tempChunkHash.resize(CHUNK_HASH_SIZE);
                memcpy(&tempChunkHash[0], tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                request.hash_.push_back(tempChunkHash);
                batchChunk.push_back(tempChunk);
                batchHash.push_back(tempChunkHash);
                memcpy(batchChunkLogicData_charBuffer + currentBatchSize, &tempChunk.chunk.logicDataSize, sizeof(int));
                currentBatchSize += sizeof(int);
                memcpy(batchChunkLogicData_charBuffer + currentBatchSize, tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize);
                currentBatchSize += tempChunk.chunk.logicDataSize;
            }
        }
        string batchChunkLogicData;
        batchChunkLogicData.resize(currentBatchSize);
        memcpy(&batchChunkLogicData[0], batchChunkLogicData_charBuffer, currentBatchSize);
        if (batchChunk.empty())
            continue;
        if (!this->request(batchChunkLogicData, request.signature_)) {
            cerr << "POWClient : sgx request failed" << endl;
            exit(1);
        }
        senderObj->sendEnclaveSignedHash(request, lists, netstatus);
        if (netstatus != SUCCESS) {
            cerr << "POWClient : send pow signed hash error" << endl;
            exit(1);
        }

        cout << "POWClient : Server need " << lists.size() << " over all " << batchChunk.size() << endl;

        for (auto it : lists) {
            batchChunk[it].chunk.type = CHUNK_TYPE_NEED_UPLOAD;
        }
        lists.clear();
        for (int i = 0; i < batchChunk.size(); i++) {
            insertMQToSender(batchChunk[i]);
        }
    }
}

bool powClient::request(string& logicDataBatch, uint8_t cmac[16])
{
    sgx_status_t retval;
    if (!enclave_trusted) {
        cerr << "POWClient : can do a request before pow_enclave trusted" << endl;
        return false;
    }

    uint32_t srcLen = logicDataBatch.length();
    sgx_status_t status;
    uint8_t* src = new uint8_t[logicDataBatch.length()];

    if (src == nullptr) {
        cerr << "mem error" << endl;
        return false;
    }
    memcpy(src, &logicDataBatch[0], logicDataBatch.length());
    status = ecall_calcmac(_eid, &retval, &_ctx, SGX_RA_KEY_SK, src, srcLen, cmac);
    if (status != SGX_SUCCESS) {
        cerr << "POWClient : ecall failed" << endl;
        return false;
    }

    return true;
}

powClient::powClient(Sender* senderObjTemp)
{
    updated = 0;
    enclave_trusted = false;
    _ctx = 0xdeadbeef;
    senderObj = senderObjTemp;
    cryptoObj = new CryptoPrimitive();
    if (!this->do_attestation()) {
        exit(1);
    } else {
        cerr << "Client : powClient remote attestation passed" << endl;
    }
}

bool powClient::do_attestation()
{
    sgx_status_t status, sgxrv, pse_status;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t* msg2;
    sgx_ra_msg3_t* msg3;
    ra_msg4_t* msg4 = NULL;
    uint32_t msg0_extended_epid_group_id = 0;
    uint32_t msg3Size;

    string enclaveName = config.getPOWEnclaveName();
    cerr << "PowClient start to create enclave" << endl;
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, &_token, &updated, &_eid, 0);
    cerr << "PowClient create enclave done" << endl;
    if (status != SGX_SUCCESS) {
        cerr << "POWClient : Can not launch pow_enclave : " << enclaveName << endl;
        printf("%08x", status);
        return false;
    } else {
        cerr << "POWClient : create pow enclave success" << endl;
    }

    status = enclave_ra_init(_eid, &sgxrv, def_service_public_key, false, &_ctx, &pse_status);
    if (status != SGX_SUCCESS) {
        cerr << "POWClient : pow_enclave ra init failed : " << status << endl;
        return false;
    } else {
        cerr << "POWClient : pow_enclave ra init success : " << status << endl;
    }

    if (sgxrv != SGX_SUCCESS) {
        cerr << "POWClient : sgx ra init failed : " << sgxrv << endl;
        return false;
    } else {
        cerr << "POWClient : sgx ra init success : " << sgxrv << endl;
    }

    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "POWClient : sgx get epid failed : " << status << endl;
        return false;
    } else if (msg0_extended_epid_group_id != 0) {
        cerr << "POWClient : sgx get epid error, epid =  " << msg0_extended_epid_group_id << endl;
    }

    /* Generate msg1 */

    status = sgx_ra_get_msg1(_ctx, _eid, sgx_ra_get_ga, &msg1);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "POWClient : sgx error get msg1" << status << endl;
        return false;
    }

    // cerr << "PowClient : generate msg 1 success, data: " << endl;
    // PRINT_BYTE_ARRAY(stderr, &msg1, sizeof(msg1));

    int netstatus;
    if (!senderObj->sendSGXmsg01(msg0_extended_epid_group_id, msg1, msg2, netstatus)) {
        cerr << "POWClient : send msg01 error : " << netstatus << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    } else {
        cerr << "POWClient : Send msg01 and Recv msg2 success" << endl;
    }

    // cerr << "PowClient : recv msg 2 success, data: " << endl;
    // fprintf(stderr, "MSG2 gb - ");
    // PRINT_BYTE_ARRAY(stderr, &(msg2->g_b), sizeof(msg2->g_b));

    // fprintf(stderr, "MSG2 spid - ");
    // PRINT_BYTE_ARRAY(stderr, &(msg2->spid), sizeof(msg2->spid));

    // fprintf(stderr, "MSG2 quote_type : %hx\n", msg2->quote_type);

    // fprintf(stderr, "MSG2 kdf_id : %hx\n", msg2->kdf_id);

    // fprintf(stderr, "MSG2 sign_gb_ga - ");
    // PRINT_BYTE_ARRAY(stderr, &(msg2->sign_gb_ga),
    //     sizeof(msg2->sign_gb_ga));

    // fprintf(stderr, "MSG2 mac - ");
    // PRINT_BYTE_ARRAY(stderr, &(msg2->mac), sizeof(msg2->mac));

    // fprintf(stderr, "MSG2 sig_rl - %d\n", msg2->sig_rl_size);
    // PRINT_BYTE_ARRAY(stderr, &(msg2->sig_rl), msg2->sig_rl_size);

    /* Process Msg2, Get Msg3  */

    status = sgx_ra_proc_msg2(_ctx, _eid, sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2, sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size, &msg3, &msg3Size);

    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "POWClient : sgx_ra_proc_msg2 : " << status << endl;
        return false;
    } else {
        cerr << "POWClient : process msg2 success" << endl;
    }

    // cerr << "PowClient : generate msg 3 success, data: " << endl;
    // PRINT_BYTE_ARRAY(stderr, msg3, msg3Size);

    cerr << "msg3Size = " << msg3Size << endl;
    if (!senderObj->sendSGXmsg3(msg3, msg3Size, msg4, netstatus)) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "POWClient : error send msg3 & get back msg4: " << netstatus << endl;
        return false;
    } else {
        cerr << "POWClient : send msg3 and Recv msg4 success" << endl;
    }

    if (msg4->status) {
        cerr << "POWClient : Enclave TRUSTED" << endl;
    } else if (!msg4->status) {
        cerr << "POWClient : Enclave NOT TRUSTED" << endl;

        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }

    enclave_trusted = msg4->status;

    return true;
}

bool powClient::editJobDoneFlag()
{
    inputMQ.done_ = true;
    if (inputMQ.done_) {
        return true;
    } else {
        return false;
    }
}

bool powClient::insertMQFromEncoder(Data_t& newChunk)
{
    return inputMQ.push(newChunk);
}

bool powClient::extractMQFromEncoder(Data_t& newChunk)
{
    return inputMQ.pop(newChunk);
}

bool powClient::insertMQToSender(Data_t& newChunk)
{
    return senderObj->insertMQFromPow(newChunk);
}