#include "../include/powClient.hpp"
#include <sys/time.h>

using namespace std;

extern Configure config;

struct timeval timestartPowClient;
struct timeval timeendPowClient;

void print(const char* str, uint32_t len)
{
    cerr << str << endl;
    uint8_t* array = (uint8_t*)str;
    for (int i = 0; i < len - 1; i++) {
        printf("0x%x, ", array[i]);
        if (i % 8 == 7)
            printf("\n");
    }
    printf("0x%x ", array[len - 1]);
    printf("\n=====================================\n");
}

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
#if SYSTEM_BREAK_DOWN == 1
    double powEnclaveCaluationTime = 0;
    double powExchangeinofrmationTime = 0;
    long diff;
    double second;
#endif
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
    bool powRequestStatus = false;

    while (true) {

        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            jobDoneFlag = true;
        }
        if (extractMQFromEncoder(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                insertMQToSender(tempChunk);
                continue;
            } else {
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
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartPowClient, NULL);
#endif
            powRequestStatus = this->request(batchChunkLogicData, request.signature_);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendPowClient, NULL);
            diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
            second = diff / 1000000.0;
            powEnclaveCaluationTime += second;
#endif
            if (!powRequestStatus) {
                cerr << "PowClient : sgx request failed" << endl;
                break;
            }
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartPowClient, NULL);
#endif
            senderObj->sendEnclaveSignedHash(request, lists, netstatus);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendPowClient, NULL);
            diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
            second = diff / 1000000.0;
            powExchangeinofrmationTime += second;
#endif
            if (netstatus != SUCCESS) {
                cerr << "PowClient : send pow signed hash error" << endl;
                break;
            } else {
                // cerr << "PowClient : send pow signed hash for " << currentBatchChunkNumber << " chunks success" << endl;
                int totalNeedChunkNumber = lists.size();
                // cout << "PowClient : Server need " << lists.size() << " over all " << batchChunk.size() << endl;
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
            break;
        }
    }
    if (!senderObj->editJobDoneFlag()) {
        cerr << "PowClient : error to set job done flag for sender" << endl;
    } else {
        cerr << "PowClient : pow thread job done, set job done flag for sender done, exit now" << endl;
    }
#if SYSTEM_BREAK_DOWN == 1
    cout << "PowClient : enclave compute work time = " << powEnclaveCaluationTime << " s" << endl;
    cout << "PowClient : exchange status to SSP time = " << powExchangeinofrmationTime << " s" << endl;
    cout << "PowClient : Total work time = " << powExchangeinofrmationTime + powEnclaveCaluationTime << " s" << endl;
#endif
    free(batchChunkLogicData_charBuffer);
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

    status = ecall_calcmac(_eid, &retval, src, srcLen, cmac);
    delete[] src;
    if (retval != SGX_SUCCESS) {
        cerr << "PowClient : ecall failed" << endl;
        return false;
    }
    return true;
}

bool powClient::loadSealedData()
{
    std::ifstream sealDataFile;
    if (sealDataFile.is_open()) {
        sealDataFile.close();
    }
    sealDataFile.open("pow-enclave.sealed", std::ios::binary);
    if (!sealDataFile.is_open()) {
        cerr << "PowClient : sealed init not success, sealed data not exist" << endl;
        return false;
    } else {
        sealDataFile.seekg(0, ios_base::end);
        int sealedDataLength = sealDataFile.tellg();
        sealDataFile.seekg(0, ios_base::beg);
        char inPutDataBuffer[sealedDataLength];
        sealDataFile.read(inPutDataBuffer, sealedDataLength);
        if (sealDataFile.gcount() != sealedDataLength) {
            cerr << "PowClient : read sealed file error" << endl;
            return false;
        } else {
            sealDataFile.close();
            memcpy(sealed_buf, inPutDataBuffer, sealedDataLength);
            cerr << "PowClient : read sealed file size = " << sealedDataLength << endl;
            return true;
        }
    }
}

bool powClient::powEnclaveSealedInit()
{
    sgx_status_t ret = SGX_SUCCESS;
    string enclaveName = config.getPOWEnclaveName();
    sgx_status_t retval;
    ret = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, NULL, NULL, &_eid, NULL);
    if (ret != SGX_SUCCESS) {
        cerr << "PowClient : create enclave error, eid = " << _eid << endl;
        sgx_destroy_enclave(_eid);
        sgxErrorReport(ret);
        return false;
    } else {
        cerr << "PowClient : create enclave done, eid = " << _eid << endl;
        ret = enclave_sealed_init(_eid, &retval, (uint8_t*)sealed_buf);
        cerr << "PowClient : unseal data size = " << sealed_len << "\t retval = " << retval << "\t status = " << ret << endl;
        if (ret == SGX_SUCCESS) {
            cerr << "PowClient : unseal data ecall success, status = " << ret << endl;
            if (retval != 0) {
                cerr << "PowClient : unseal data error, retval = " << retval << endl;
                sgx_destroy_enclave(_eid);
                return false;
            } else {
                return true;
            }
        } else {
            cerr << "PowClient : unseal data ecall error, status = " << ret << endl;
            sgxErrorReport(ret);
            sgx_destroy_enclave(_eid);
            return false;
        }
    }
}

bool powClient::powEnclaveSealedColse()
{
    sgx_status_t ret;
    sgx_status_t retval;
    ret = enclave_sealed_close(_eid, &retval, (uint8_t*)sealed_buf);
    if (ret != SGX_SUCCESS) {
        cerr << "PowClient : seal data ecall error, status = " << endl;
        sgxErrorReport(ret);
        return false;
    } else {
        if (retval != 0) {
            cerr << "PowClient : unseal data ecall return error, return value = " << retval << endl;
            return false;
        } else {
            return true;
        }
    }
}

bool powClient::outputSealedData()
{
    std::ofstream sealDataFile;
    if (sealDataFile.is_open()) {
        sealDataFile.close();
    }
    sealDataFile.open("pow-enclave.sealed", std::ofstream::out | std::ios::binary);
    if (sealDataFile.is_open()) {
        char outPutDataBuffer[sealed_len];
        memcpy(outPutDataBuffer, sealed_buf, sealed_len);
        sealDataFile.write(outPutDataBuffer, sealed_len);
        sealDataFile.close();
        return true;
    } else {
        return false;
    }
}

powClient::powClient(Sender* senderObjTemp)
{
    inputMQ_ = new messageQueue<Data_t>;
    enclave_trusted = false;
    _ctx = 0xdeadbeef;
    senderObj = senderObjTemp;
    cryptoObj = new CryptoPrimitive();
    sealed_len = sizeof(sgx_sealed_data_t) + sizeof(sgx_ra_key_128_t);
    cout << "PowClient : sealed size = " << sealed_len << endl;
    sealed_buf = (char*)malloc(sealed_len);
    memset(sealed_buf, -1, sealed_len);
    if (loadSealedData() == true) {
        cerr << "PowClient : load sealed data success" << endl;
        if (powEnclaveSealedInit() == true) {
            cerr << "PowClient : enclave init via sealed data done" << endl;
            startMethod = 1;
            if (senderObj->sendLogInMessage()) {
                cerr << "PowClient : login to storage service provider success" << endl;
            } else {
                cerr << "PowClient : login to storage service provider error" << endl;
            }
        } else {
            cerr << "PowClient : enclave init via sealed data error" << endl;
            sgx_destroy_enclave(_eid);
            exit(0);
        }
    } else {
        senderObj->sendLogOutMessage();
        if (senderObj->sendLogInMessage()) {
            cerr << "PowClient : login to storage service provider success" << endl;
        } else {
            cerr << "PowClient : login to storage service provider error" << endl;
        }
        if (!this->do_attestation()) {
            return;
        } else {
            sgx_status_t retval;
            sgx_status_t status;
            status = ecall_setSessionKey(_eid, &retval, &_ctx, SGX_RA_KEY_SK);
            if (status != SGX_SUCCESS) {
                cerr << "PowClient : ecall set session key failed, eid = " << _eid << endl;
                exit(0);
            } else {
                startMethod = 2;
                cerr << "PowClient : ecall set session key success, eid = " << _eid << endl;
            }
        }
    }
}

powClient::~powClient()
{
    inputMQ_->~messageQueue();
    delete inputMQ_;
    delete cryptoObj;
    if (startMethod == 2) {
        if (powEnclaveSealedColse() == true) {
            cout << "PowClient : enclave sealing done" << endl;
            if (outputSealedData() == true) {
                cout << "PowClient : enclave sealing write out done" << endl;
            } else {
                cerr << "PowClient : enclave sealing write out error" << endl;
            }
        } else {
            cerr << "PowClient : enclave sealing error" << endl;
        }
    }
    free(sealed_buf);
    sgx_status_t ret;
    ret = sgx_destroy_enclave(_eid);
    if (ret != SGX_SUCCESS) {
        cerr << "PowClient : enclave clean up error" << endl;
    } else {
        cerr << "PowClient : enclave clean up" << endl;
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
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, NULL, NULL, &_eid, NULL);
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