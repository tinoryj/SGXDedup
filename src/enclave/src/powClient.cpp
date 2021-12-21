#include "powClient.hpp"
#include "sgxErrorSupport.h"
// #include <sys/time.h>

using namespace std;

extern Configure config;

#if SYSTEM_BREAK_DOWN == 1
struct timeval timestartPowClient;
struct timeval timeendPowClient;
#endif

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
    double powExchangeInofrmationTime = 0;
    double powBuildHashListTime = 0;
    long diff;
    double second;
#endif
    vector<Data_t> batchChunk;
    uint64_t powBatchSize = config.getPOWBatchSize();
    u_char* batchChunkLogicDataCharBuffer;
    batchChunkLogicDataCharBuffer = (u_char*)malloc(sizeof(u_char) * (MAX_CHUNK_SIZE + sizeof(int)) * powBatchSize);
    memset(batchChunkLogicDataCharBuffer, 0, sizeof(u_char) * (MAX_CHUNK_SIZE + sizeof(int)) * powBatchSize);
    Data_t tempChunk;
    int netstatus;
    int currentBatchChunkNumber = 0;
    bool jobDoneFlag = false;
    uint32_t currentBatchSize = 0;
    batchChunk.clear();
    bool powRequestStatus = false;

    while (true) {

        if (inputMQ_->done_ && inputMQ_->isEmpty()) {
            jobDoneFlag = true;
        }
        if (extractMQ(tempChunk)) {
            if (tempChunk.dataType == DATA_TYPE_RECIPE) {
                senderObj_->insertMQ(tempChunk);
                continue;
            } else {
                batchChunk.push_back(tempChunk);
                memcpy(batchChunkLogicDataCharBuffer + currentBatchSize, &tempChunk.chunk.logicDataSize, sizeof(int));
                currentBatchSize += sizeof(int);
                memcpy(batchChunkLogicDataCharBuffer + currentBatchSize, tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize);
                currentBatchSize += tempChunk.chunk.logicDataSize;
                currentBatchChunkNumber++;
            }
        }
        if (currentBatchChunkNumber == powBatchSize || jobDoneFlag) {
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartPowClient, NULL);
#endif
            uint8_t clientMac[16];
            uint8_t chunkHashList[currentBatchChunkNumber * SYSTEM_CIPHER_SIZE];
            memset(chunkHashList, 0, currentBatchChunkNumber * SYSTEM_CIPHER_SIZE);
            powRequestStatus = this->request(batchChunkLogicDataCharBuffer, currentBatchSize, clientMac, chunkHashList);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendPowClient, NULL);
            diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
            second = diff / 1000000.0;
            powEnclaveCaluationTime += second;
#endif
            if (!powRequestStatus) {
                cerr << "PowClient : sgx request failed" << endl;
                break;
            } else {
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartPowClient, NULL);
#endif
                for (int i = 0; i < currentBatchChunkNumber; i++) {
                    memcpy(batchChunk[i].chunk.chunkHash, chunkHashList + i * SYSTEM_CIPHER_SIZE, SYSTEM_CIPHER_SIZE);
                }
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendPowClient, NULL);
                diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
                second = diff / 1000000.0;
                powBuildHashListTime += second;
#endif
            }
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestartPowClient, NULL);
#endif
            u_char serverResponse[sizeof(int) + sizeof(bool) * currentBatchChunkNumber];
            senderObj_->sendEnclaveSignedHash(clientMac, chunkHashList, currentBatchChunkNumber, serverResponse, netstatus);
#if SYSTEM_DEBUG_FLAG == 1
            cerr << "PowClient : send signed hash list data = " << endl;
            PRINT_BYTE_ARRAY_POW_CLIENT(stderr, clientMac, 16);
            // PRINT_BYTE_ARRAY_POW_CLIENT(stderr, chunkHashList, currentBatchChunkNumber * SYSTEM_CIPHER_SIZE);
#endif
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeendPowClient, NULL);
            diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
            second = diff / 1000000.0;
            powExchangeInofrmationTime += second;
#endif
            if (netstatus != SUCCESS) {
                cerr << "PowClient : server pow signed hash verify error, client mac = " << endl;
                PRINT_BYTE_ARRAY_POW_CLIENT(stderr, clientMac, 16);
                PRINT_BYTE_ARRAY_POW_CLIENT(stderr, chunkHashList, SYSTEM_CIPHER_SIZE);
                break;
            } else {
                int totalNeedChunkNumber;
                memcpy(&totalNeedChunkNumber, serverResponse, sizeof(int));
                bool requiredChunksList[currentBatchChunkNumber];
                memcpy(requiredChunksList, serverResponse + sizeof(int), sizeof(bool) * currentBatchChunkNumber);
#if SYSTEM_DEBUG_FLAG == 1
                cerr << "PowClient : send pow signed hash for " << currentBatchChunkNumber << " chunks success, Server need " << totalNeedChunkNumber << " over all " << batchChunk.size() << endl;
#endif
                for (int i = 0; i < totalNeedChunkNumber; i++) {
                    if (requiredChunksList[i] == true) {
                        batchChunk[i].chunk.type = CHUNK_TYPE_NEED_UPLOAD;
                    }
                }
                int batchChunkSize = batchChunk.size();
                for (int i = 0; i < batchChunkSize; i++) {
                    senderObj_->insertMQ(batchChunk[i]);
                }
            }
            currentBatchChunkNumber = 0;
            currentBatchSize = 0;
            batchChunk.clear();
        }
        if (jobDoneFlag) {
            break;
        }
    }
    if (!senderObj_->editJobDoneFlag()) {
        cerr << "PowClient : error to set job done flag for sender" << endl;
    }
#if SYSTEM_BREAK_DOWN == 1
    cout << "PowClient : enclave compute work time = " << powEnclaveCaluationTime << " s" << endl;
    cout << "PowClient : build hash list and insert hash to chunk time = " << powBuildHashListTime << " s" << endl;
    cout << "PowClient : exchange status to storage service provider time = " << powExchangeInofrmationTime << " s" << endl;
    cout << "PowClient : Total work time = " << powExchangeInofrmationTime + powEnclaveCaluationTime + powBuildHashListTime << " s" << endl;
#endif
    free(batchChunkLogicDataCharBuffer);
    return;
}

bool powClient::request(u_char* logicDataBatchBuffer, uint32_t bufferSize, uint8_t cmac[16], uint8_t* chunkHashList)
{
    sgx_status_t status, retval;
    status = ecall_calcmac(eid_, &retval, (uint8_t*)logicDataBatchBuffer, bufferSize, cmac, chunkHashList);
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : ecall failed, status = " << endl;
        sgxErrorReport(status);
        return false;
    } else if (retval != SGX_SUCCESS) {
        cerr << "PowClient : pow compute error, retval = " << endl;
        sgxErrorReport(retval);
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
#if SYSTEM_LOG_FLAG == 1
        cerr << "PowClient : no sealed infomation, start remote attestation login" << endl;
#endif
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
            memcpy(sealedBuffer_, inPutDataBuffer, sealedDataLength);
            return true;
        }
    }
}

bool powClient::powEnclaveSealedInit()
{
    sgx_status_t status = SGX_SUCCESS;
    string enclaveName = config.getPOWEnclaveName();
    sgx_status_t retval;
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartEnclave;
    struct timeval timeendEnclave;
    long diff;
    double second;
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartEnclave, NULL);
#endif
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, NULL, NULL, &eid_, NULL);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendEnclave, NULL);
    diff = 1000000 * (timeendEnclave.tv_sec - timestartEnclave.tv_sec) + timeendEnclave.tv_usec - timestartEnclave.tv_usec;
    second = diff / 1000000.0;
    cout << "PowClient : create enclave time = " << setprecision(8) << second << " s" << endl;
#endif
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : create enclave error, eid = " << eid_ << endl;
        sgx_destroy_enclave(eid_);
        sgxErrorReport(status);
        return false;
    } else {
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "PowClient : create enclave done" << endl;
#endif
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartEnclave, NULL);
#endif
        status = enclave_sealed_init(eid_, &retval, (uint8_t*)sealedBuffer_);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendEnclave, NULL);
        diff = 1000000 * (timeendEnclave.tv_sec - timestartEnclave.tv_sec) + timeendEnclave.tv_usec - timestartEnclave.tv_usec;
        second = diff / 1000000.0;
        cout << "PowClient : sealed init enclave time = " << setprecision(8) << second << " s" << endl;
#endif
#if SYSTEM_DEBUG_FLAG == 1
        cerr << "PowClient : unseal data size = " << sealedLen_ << "\t retval = " << retval << "\t status = " << status << endl;
#endif
        if (status == SGX_SUCCESS) {
#if SYSTEM_DEBUG_FLAG == 1
            cerr << "PowClient : unseal data ecall success, status = " << status << endl;
#endif
            if (retval != SGX_SUCCESS) {
                cerr << "PowClient : unseal data error, retval = " << retval << endl;
                sgx_destroy_enclave(eid_);
                return false;
            } else {
                return true;
            }
        } else {
            cerr << "PowClient : unseal data ecall error, status = " << status << endl;
            sgxErrorReport(status);
            sgx_destroy_enclave(eid_);
            return false;
        }
    }
}

bool powClient::powEnclaveSealedColse()
{
    sgx_status_t status;
    sgx_status_t retval;
    status = enclave_sealed_close(eid_, &retval, (uint8_t*)sealedBuffer_);
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : seal data ecall error, status = " << endl;
        sgxErrorReport(status);
        return false;
    } else {
        if (retval != SGX_SUCCESS) {
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
        char outPutDataBuffer[sealedLen_];
        memcpy(outPutDataBuffer, sealedBuffer_, sealedLen_);
        sealDataFile.write(outPutDataBuffer, sealedLen_);
        sealDataFile.close();
        return true;
    } else {
        return false;
    }
}

powClient::powClient(Sender* senderObjTemp)
{
    inputMQ_ = new messageQueue<Data_t>;
    enclaveIsTrusted_ = false;
    ctx_ = 0xdeadbeef;
    senderObj_ = senderObjTemp;
    cryptoObj_ = new CryptoPrimitive();
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartEnclave;
    struct timeval timeendEnclave;
    long diff;
    double second;
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartPowClient, NULL);
#endif
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartEnclave, NULL);
#endif
    bool remoteAttestationStatus = this->startEnclave();
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendEnclave, NULL);
    diff = 1000000 * (timeendEnclave.tv_sec - timestartEnclave.tv_sec) + timeendEnclave.tv_usec - timestartEnclave.tv_usec;
    second = diff / 1000000.0;
    cout << "PowClient : remote attestation init total work time = " << setprecision(8) << second << " s" << endl;
#endif
    sgx_status_t status, retval;
    status = ecall_setSessionKey(eid_, &retval, &ctx_, SGX_RA_KEY_SK);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendPowClient, NULL);
    diff = 1000000 * (timeendPowClient.tv_sec - timestartPowClient.tv_sec) + timeendPowClient.tv_usec - timestartPowClient.tv_usec;
    second = diff / 1000000.0;
    cout << "PowClient : enclave init time = " << setprecision(8) << second << " s" << endl;
#endif
}

powClient::~powClient()
{
    inputMQ_->~messageQueue();
    delete inputMQ_;
    delete cryptoObj_;
    sgx_status_t ret;
    ret = sgx_destroy_enclave(eid_);
    if (ret != SGX_SUCCESS) {
        cerr << "PowClient : enclave clean up error" << endl;
    }
}

bool powClient::startEnclave()
{
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartEnclave;
    struct timeval timeendEnclave;
    long diff;
    double second;
#endif
    sgx_status_t status;
    string enclaveName = config.getPOWEnclaveName();
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartEnclave, NULL);
#endif
    status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, NULL, NULL, &eid_, NULL);
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timeendEnclave, NULL);
    diff = 1000000 * (timeendEnclave.tv_sec - timestartEnclave.tv_sec) + timeendEnclave.tv_usec - timestartEnclave.tv_usec;
    second = diff / 1000000.0;
    cout << "PowClient : create enclave time = " << setprecision(8) << second << " s" << endl;
#endif
#if SYSTEM_LOG_FLAG == 1
    cerr << "PowClient : create pow enclave done" << endl;
#endif
    if (status != SGX_SUCCESS) {
        cerr << "PowClient : Can not launch pow_enclave : " << enclaveName << endl;
        sgxErrorReport(status);
        return false;
    }
#if SYSTEM_BREAK_DOWN == 1
    gettimeofday(&timestartEnclave, NULL);
#endif
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

bool powClient::insertMQ(Data_t& newChunk)
{
    return inputMQ_->push(newChunk);
}

bool powClient::extractMQ(Data_t& newChunk)
{
    return inputMQ_->pop(newChunk);
}