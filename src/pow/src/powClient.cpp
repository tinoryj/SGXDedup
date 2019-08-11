#include "../include/powClient.hpp"
#include <sys/time.h>

using namespace std;

extern Configure config;

struct timeval timestartPowClient;
struct timeval timeendPowClient;

void PRINT_BYTE_ARRAY_POW_CLIENT(
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
    gettimeofday(&timestartPowClient, NULL);
    vector<Data_t> batchChunk;
    vector<string> batchHash;
    uint64_t powBatchSize = config.getPOWBatchSize();
    u_char* batchChunkLogicData_charBuffer;
    batchChunkLogicData_charBuffer = (u_char*)malloc(sizeof(u_char) * (MAX_CHUNK_SIZE + sizeof(int)) * powBatchSize);
    memset(batchChunkLogicData_charBuffer, 0, sizeof(u_char) * (MAX_CHUNK_SIZE + sizeof(int)) * powBatchSize);
    powSignedHash_t request;
    RequiredChunk_t lists;
    Data_t tempChunk;
    int netstatus;
    int currentBatchChunkNumber = 0;
    bool jobDoneFlag = false;
    uint64_t currentBatchSize = 0;
    batchChunk.clear();
    batchHash.clear();
    request.hash_.clear();

    while (true) {

        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            jobDoneFlag = true;
        }
        if (extractMQFromEncoder(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                // cerr << "PowClient : get file recipe head frome message queue, file size = " << tempChunk.recipe.fileRecipeHead.fileSize << " file chunk number = " << tempChunk.recipe.fileRecipeHead.totalChunkNumber << endl;
                // PRINT_BYTE_ARRAY_POW_CLIENT(stderr, tempChunk.recipe.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
                insertMQToSender(tempChunk);
                continue;
            } else {
                // cout << "PowClient : current extract  chunk ID = " << tempChunk.chunk.ID << " chunk data size = " << tempChunk.chunk.logicDataSize << endl;
                // cout << setw(6) << "chunk hash = " << endl;
                // PRINT_BYTE_ARRAY_POW_CLIENT(stdout, tempChunk.chunk.chunkHash, CHUNK_HASH_SIZE);
                // cout << setw(6) << "chunk key = " << endl;
                // PRINT_BYTE_ARRAY_POW_CLIENT(stdout, tempChunk.chunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
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
                currentBatchChunkNumber++;
            }
        }
        if (currentBatchChunkNumber == powBatchSize || jobDoneFlag) {
            string batchChunkLogicData;
            batchChunkLogicData.resize(currentBatchSize);
            memcpy(&batchChunkLogicData[0], batchChunkLogicData_charBuffer, currentBatchSize);

            if (!this->request(batchChunkLogicData, request.signature_)) {
                cerr << "PowClient : sgx request failed" << endl;
                exit(1);
            }
            senderObj->sendEnclaveSignedHash(request, lists, netstatus);
            if (netstatus != SUCCESS) {
                cerr << "PowClient : send pow signed hash error" << endl;
                exit(1);
            } else {
                cerr << "PowClient : send pow signed hash for " << currentBatchChunkNumber << " chunks success" << endl;
                int totalNeedChunkNumber = lists.size();
                cout << "PowClient : Server need " << lists.size() << " over all " << batchChunk.size() << endl;
                for (int i = 0; i < totalNeedChunkNumber; i++) {
                    batchChunk[lists[i]].chunk.type = CHUNK_TYPE_NEED_UPLOAD;
                }
                lists.clear();
                int batchChunkSize = batchChunk.size();
                for (int i = 0; i < batchChunkSize; i++) {
                    insertMQToSender(batchChunk[i]);
                }
            }
            currentBatchChunkNumber = 0;
            currentBatchSize = 0;
            batchChunk.clear();
            batchHash.clear();
            request.hash_.clear();
        }
        if (jobDoneFlag) {
            if (!senderObj->editJobDoneFlag()) {
                cerr << "PowClient : error to set job done flag for sender" << endl;
            } else {
                cerr << "PowClient : pow thread job done, set job done flag for sender done, exit now" << endl;
            }
            break;
        }
    }
    gettimeofday(&timeendPowClient, NULL);
    long diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
    double second = diff / 1000000.0;
    printf("PowClient : thread work time is %ld us = %lf s\n", diff, second);
    return;
}

bool powClient::request(string& logicDataBatch, uint8_t cmac[16])
{
    sgx_status_t retval;
    uint32_t srcLen = logicDataBatch.length();
    sgx_status_t status;
    uint8_t* src = new uint8_t[logicDataBatch.length()];

    if (src == nullptr) {
        cerr << "PowClient : mem error at request -> create ecall src buffer" << endl;
        return false;
    }
    memcpy(src, &logicDataBatch[0], logicDataBatch.length());

    status = ecall_calcmac(_eid, &retval, &_ctx, SGX_RA_KEY_SK, src, srcLen, cmac);
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : ecall failed" << endl;
        return false;
    }
    return true;
}

powClient::powClient(Sender* senderObjTemp)
{
    inputMQ_ = new messageQueue<Data_t>(config.get_Data_t_MQSize());
    updated = 0;
    enclave_trusted = false;
    _ctx = 0xdeadbeef;
    senderObj = senderObjTemp;
    cryptoObj = new CryptoPrimitive();
    if (!this->do_attestation()) {
        exit(1);
    }
}

powClient::~powClient()
{
    delete inputMQ_;
    delete cryptoObj;
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
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, &_token, &updated, &_eid, 0);
    cerr << "PowClient : create pow enclave done" << endl;
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : Can not launch pow_enclave : " << enclaveName << endl;
        printf("%08x", status);
        return false;
    }

    status = enclave_ra_init(_eid, &sgxrv, def_service_public_key, false, &_ctx, &pse_status);
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : pow_enclave ra init failed : " << status << endl;
        return false;
    }

    if (sgxrv != SGX_SUCCESS) {
        cerr << "PowClient : sgx ra init failed : " << sgxrv << endl;
        return false;
    }

    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "PowClient : sgx get epid failed : " << status << endl;
        return false;
    }
    /* Generate msg1 */

    status = sgx_ra_get_msg1(_ctx, _eid, sgx_ra_get_ga, &msg1);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "PowClient : sgx error get msg1" << status << endl;
        return false;
    }

    int netstatus;
    if (!senderObj->sendSGXmsg01(msg0_extended_epid_group_id, msg1, msg2, netstatus)) {
        cerr << "PowClient : send msg01 error : " << netstatus << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }

    status = sgx_ra_proc_msg2(_ctx, _eid, sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2, sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size, &msg3, &msg3Size);

    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "PowClient : sgx_ra_proc_msg2 : " << status << endl;
        return false;
    }

    if (!senderObj->sendSGXmsg3(msg3, msg3Size, msg4, netstatus)) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "PowClient : error send msg3 & get back msg4: " << netstatus << endl;
        return false;
    }
    if (msg4->status) {
        cerr << "PowClient : Enclave TRUSTED" << endl;
    } else if (!msg4->status) {
        cerr << "PowClient : Enclave NOT TRUSTED" << endl;
        enclave_ra_close(_eid, &sgxrv, _ctx);
        return false;
    }

    enclave_trusted = msg4->status;

    return true;
}

bool powClient::editJobDoneFlag()
{
    inputMQ_->done_ = true;
    if (inputMQ_->done_) {
        return true;
    } else {
        return false;
    }
}

bool powClient::insertMQFromEncoder(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool powClient::extractMQFromEncoder(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}

bool powClient::insertMQToSender(Data_t& newChunk)
{
    return senderObj->insertMQFromPow(newChunk);
}