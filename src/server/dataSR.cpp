
#include <dataSR.hpp>
#include <sys/times.h>

extern Configure config;

void PRINT_BYTE_ARRAY_DATA_SR(
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

DataSR::DataSR(StorageCore* storageObj, DedupCore* dedupCoreObj, powServer* powServerObj, ssl* powSecurityChannelTemp, ssl* dataSecurityChannelTemp)
{
    restoreChunkBatchNumber_ = config.getSendChunkBatchSize();
    storageObj_ = storageObj;
    dedupCoreObj_ = dedupCoreObj;
    powServerObj_ = powServerObj;
    cryptoObj_ = new CryptoPrimitive();
    keyExchangeKeySetFlag_ = false;
    powSecurityChannel_ = powSecurityChannelTemp;
    dataSecurityChannel_ = dataSecurityChannelTemp;
    keyRegressionCurrentTimes_ = config.getKeyRegressionMaxTimes();
#if SYSTEM_DEBUG_FLAG == 1
    cerr << " DataSR : key regression current count = " << keyRegressionCurrentTimes_ << endl;
#endif
    memset(keyExchangeKey_, 0, 32);
}

DataSR::~DataSR()
{
    delete cryptoObj_;
}

void DataSR::runData(SSL* sslConnection)
{
    int recvSize = 0;
    int sendSize = 0;
    char recvBuffer[NETWORK_MESSAGE_DATA_SIZE];
    char sendBuffer[NETWORK_MESSAGE_DATA_SIZE];
    // double totalSaveChunkTime = 0;
    uint32_t startID_ = 0;
    uint32_t endID_ = 0;
    Recipe_t restoredFileRecipe_;
    uint32_t totalRestoredChunkNumber_ = 0;
    char* restoredRecipeList;
    uint64_t recipeSize = 0;
#if SYSTEM_BREAK_DOWN == 1
    bool uploadFlag = false;
    struct timeval timestartDataSR;
    struct timeval timeendDataSR;
    double saveChunkTime = 0;
    double saveRecipeTime = 0;
    double restoreChunkTime = 0;
    long diff;
    double second;
#endif
    while (true) {
        if (!dataSecurityChannel_->recv(sslConnection, recvBuffer, recvSize)) {
            cout << "DataSR : client closed socket connect, thread exit now" << endl;
#if SYSTEM_BREAK_DOWN == 1
            if (uploadFlag == true) {
                cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
            } else {
                cout << "DataSR : total restore chunk time = " << restoreChunkTime << " s" << endl;
            }
#endif
            cerr << "DataSR : data thread exit now due to client connection lost" << endl;
            if (restoredRecipeList != nullptr) {
                free(restoredRecipeList);
            }
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
#if SYSTEM_DEBUG_FLAG == 1
            cerr << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
#endif
            switch (netBody.messageType) {
            case CLIENT_EXIT: {
                netBody.messageType = SERVER_JOB_DONE_EXIT_PERMIT;
                netBody.dataSize = 0;
                sendSize = sizeof(NetworkHeadStruct_t);
                memset(sendBuffer, 0, NETWORK_MESSAGE_DATA_SIZE);
                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
#if SYSTEM_BREAK_DOWN == 1
                if (uploadFlag == true) {
                    cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                    cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
                } else {
                    cout << "DataSR : total restore chunk time = " << restoreChunkTime << " s" << endl;
                }
                storageObj_->clientExitSystemStatusOutput(uploadFlag);
#endif
#if SYSTEM_LOG_FLAG == 1
                cerr << "DataSR : data thread recv exit flag, thread exit now" << endl;
#endif
                if (restoredRecipeList != nullptr) {
                    free(restoredRecipeList);
                }
                return;
            }
            case CLIENT_UPLOAD_CHUNK: {
#if SYSTEM_BREAK_DOWN == 1
                uploadFlag = true;
                gettimeofday(&timestartDataSR, NULL);
#endif
                bool storeChunkStatus = storageObj_->storeChunks(netBody, (char*)recvBuffer + sizeof(NetworkHeadStruct_t));
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendDataSR, NULL);
                diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                second = diff / 1000000.0;
                saveChunkTime += second;
#endif
                if (!storeChunkStatus) {
                    cerr << "DedupCore : store chunks report error, server may incur internal error, thread exit" << endl;
#if SYSTEM_BREAK_DOWN == 1
                    if (uploadFlag == true) {
                        cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                        cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
                    } else {
                        cout << "DataSR : total restore chunk time = " << restoreChunkTime << " s" << endl;
                    }
                    storageObj_->clientExitSystemStatusOutput(uploadFlag);
#endif
                    cerr << "DataSR : data thread exit now due to client connection lost" << endl;
                    if (restoredRecipeList != nullptr) {
                        free(restoredRecipeList);
                    }
                    return;
                }
                break;
            }
            case CLIENT_UPLOAD_ENCRYPTED_RECIPE: {
#if SYSTEM_BREAK_DOWN == 1
                uploadFlag = true;
#endif
                int recipeListSize = netBody.dataSize;
#if SYSTEM_LOG_FLAG == 1
                cout << "DataSR : recv file recipe size = " << recipeListSize << endl;
#endif
                char* recipeListBuffer = (char*)malloc(sizeof(char) * recipeListSize + sizeof(NetworkHeadStruct_t));
                if (!dataSecurityChannel_->recv(sslConnection, recipeListBuffer, recvSize)) {
                    cout << "DataSR : client closed socket connect, recipe store failed, Thread exit now" << endl;
#if SYSTEM_BREAK_DOWN == 1
                    if (uploadFlag == true) {
                        cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                        cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
                    } else {
                        cout << "DataSR : total restore chunk time = " << restoreChunkTime << " s" << endl;
                    }
                    storageObj_->clientExitSystemStatusOutput(uploadFlag);
#endif
                    cerr << "DataSR : data thread exit now due to client connection lost" << endl;
                    if (restoredRecipeList != nullptr) {
                        free(restoredRecipeList);
                    }
                    return;
                }
                Recipe_t newFileRecipe;
                memcpy(&newFileRecipe, recipeListBuffer + sizeof(NetworkHeadStruct_t), sizeof(Recipe_t));
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartDataSR, NULL);
#endif
                storageObj_->storeRecipes((char*)newFileRecipe.fileRecipeHead.fileNameHash, (u_char*)recipeListBuffer + sizeof(NetworkHeadStruct_t), recipeListSize);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendDataSR, NULL);
                diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                second = diff / 1000000.0;
                saveRecipeTime += second;
#endif
                free(recipeListBuffer);
                break;
            }
            case CLIENT_UPLOAD_DECRYPTED_RECIPE: {
                // cout << "DataSR : current recipe size = " << recipeSize << ", toatl chunk number = " << restoredFileRecipe_.fileRecipeHead.totalChunkNumber << endl;
                uint64_t decryptedRecipeListSize = 0;
                memcpy(&decryptedRecipeListSize, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(uint64_t));
                // cout << "DataSR : process recipe list size = " << decryptedRecipeListSize << endl;
                restoredRecipeList = (char*)malloc(sizeof(char) * decryptedRecipeListSize + sizeof(NetworkHeadStruct_t));
                if (dataSecurityChannel_->recv(sslConnection, restoredRecipeList, recvSize)) {
                    NetworkHeadStruct_t tempHeader;
                    memcpy(&tempHeader, restoredRecipeList, sizeof(NetworkHeadStruct_t));
                    // cout << "DataSR : CLIENT_UPLOAD_DECRYPTED_RECIPE, recv message type " << tempHeader.messageType << ", message size = " << tempHeader.dataSize << endl;
                } else {
                    cerr << "DataSR : recv decrypted file recipe error " << endl;
                }
                cerr << "DataSR : process recipe list done" << endl;
                break;
            }
            case CLIENT_DOWNLOAD_ENCRYPTED_RECIPE: {
#if MULTI_CLIENT_UPLOAD_TEST == 1
                mutexRestore_.lock();
#endif
                bool restoreRecipeSizeStatus = storageObj_->restoreRecipesSize((char*)recvBuffer + sizeof(NetworkHeadStruct_t), recipeSize);
#if MULTI_CLIENT_UPLOAD_TEST == 1
                mutexRestore_.unlock();
#endif
                if (restoreRecipeSizeStatus) {
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = recipeSize;
                    sendSize = sizeof(NetworkHeadStruct_t);
                    memset(sendBuffer, 0, NETWORK_MESSAGE_DATA_SIZE);
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                    u_char* recipeBuffer = (u_char*)malloc(sizeof(u_char) * recipeSize);
#if MULTI_CLIENT_UPLOAD_TEST == 1
                    mutexRestore_.lock();
#endif
                    storageObj_->restoreRecipes((char*)recvBuffer + sizeof(NetworkHeadStruct_t), recipeBuffer, recipeSize);
#if MULTI_CLIENT_UPLOAD_TEST == 1
                    mutexRestore_.unlock();
#endif
                    char* sendRecipeBuffer = (char*)malloc(sizeof(char) * recipeSize + sizeof(NetworkHeadStruct_t));
                    memcpy(sendRecipeBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    memcpy(sendRecipeBuffer + sizeof(NetworkHeadStruct_t), recipeBuffer, recipeSize);
                    sendSize = sizeof(NetworkHeadStruct_t) + recipeSize;
                    dataSecurityChannel_->send(sslConnection, sendRecipeBuffer, sendSize);
                    memcpy(&restoredFileRecipe_, recipeBuffer, sizeof(Recipe_t));
#if SYSTEM_DEBUG_FLAG == 1
                    cerr << "StorageCore : send encrypted recipe list done, file size = " << restoredFileRecipe_.fileRecipeHead.fileSize << ", total chunk number = " << restoredFileRecipe_.fileRecipeHead.totalChunkNumber << endl;
#endif
                    free(sendRecipeBuffer);
                    free(recipeBuffer);
                } else {
                    netBody.messageType = ERROR_FILE_NOT_EXIST;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                    dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                }
                break;
            }
            case CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE: {
                cerr << "DataSR : start retrive chunks, chunk number = " << restoredFileRecipe_.fileRecipeHead.totalChunkNumber << endl;
                if (restoredFileRecipe_.fileRecipeHead.totalChunkNumber < restoreChunkBatchNumber_) {
                    endID_ = restoredFileRecipe_.fileRecipeHead.totalChunkNumber - 1;
                }
                while (totalRestoredChunkNumber_ != restoredFileRecipe_.fileRecipeHead.totalChunkNumber) {
                    memset(sendBuffer, 0, NETWORK_MESSAGE_DATA_SIZE);
                    int restoredChunkNumber = 0, restoredChunkSize = 0;
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartDataSR, NULL);
#endif
#if MULTI_CLIENT_UPLOAD_TEST == 1
                    mutexRestore_.lock();
#endif
                    bool restoreChunkStatus = storageObj_->restoreRecipeAndChunk(restoredRecipeList + sizeof(NetworkHeadStruct_t) + startID_ * (SYSTEM_CIPHER_SIZE + sizeof(int)), startID_, endID_, sendBuffer + sizeof(NetworkHeadStruct_t) + sizeof(int), restoredChunkNumber, restoredChunkSize);
#if MULTI_CLIENT_UPLOAD_TEST == 1
                    mutexRestore_.unlock();
#endif
                    if (restoreChunkStatus) {
                        netBody.messageType = SUCCESS;
                        memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &restoredChunkNumber, sizeof(int));
                        netBody.dataSize = sizeof(int) + restoredChunkSize;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t) + sizeof(int) + restoredChunkSize;
                        totalRestoredChunkNumber_ += restoredChunkNumber;
                        startID_ = endID_;
                        uint32_t remainChunkNumber = restoredFileRecipe_.fileRecipeHead.totalChunkNumber - totalRestoredChunkNumber_;
                        // cout << "DataSR : wait for restore chunk number = " << remainChunkNumber << ", current restored chunk number = " << restoredChunkNumber << endl;
                        if (remainChunkNumber < restoreChunkBatchNumber_) {
                            endID_ += restoredFileRecipe_.fileRecipeHead.totalChunkNumber - totalRestoredChunkNumber_;
                        } else {
                            endID_ += restoreChunkBatchNumber_;
                        }
                    } else {
                        netBody.dataSize = 0;
                        netBody.messageType = ERROR_CHUNK_NOT_EXIST;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
#if SYSTEM_BREAK_DOWN == 1
                        if (uploadFlag == true) {
                            cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                            cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
                        } else {
                            cout << "DataSR : total restore chunk time = " << restoreChunkTime << " s" << endl;
                        }
                        storageObj_->clientExitSystemStatusOutput(uploadFlag);
#endif
                        cerr << "DataSR : data thread exit now due to client connection lost" << endl;
                        if (restoredRecipeList != nullptr) {
                            free(restoredRecipeList);
                        }
                        return;
                    }
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendDataSR, NULL);
                    diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                    second = diff / 1000000.0;
                    restoreChunkTime += second;
#endif
                    dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
#if SYSTEM_LOG_FLAG == 1
                    cerr << "DataSR : send back chunks last ID = " << startID_ << endl;
#endif
                    // cerr << "DataSR : new start ID = " << startID_ << ", end ID = " << endID_ << endl;
                }
                break;
            }
            default:
                continue;
            }
        }
    }
    cerr << "DataSR : data thread exit now due to client connection lost" << endl;
    if (restoredRecipeList != nullptr) {
        free(restoredRecipeList);
    }
    return;
}

void DataSR::runPow(SSL* sslConnection)
{
    int recvSize = 0;
    int sendSize = 0;
    char recvBuffer[NETWORK_MESSAGE_DATA_SIZE];
    char sendBuffer[NETWORK_MESSAGE_DATA_SIZE];
    int clientID = -1;
    enclaveSession* currentSession;
#if SYSTEM_BREAK_DOWN == 1
    struct timeval timestartDataSR;
    struct timeval timeendDataSR;
    double verifyTime = 0;
    double dedupTime = 0;
    long diff;
    double second;
#endif
    while (true) {

        if (!powSecurityChannel_->recv(sslConnection, recvBuffer, recvSize)) {
            cout << "DataSR : client closed socket connect, Client ID = " << clientID << endl;
#if SYSTEM_BREAK_DOWN == 1
            cout << "DataSR : total pow Verify time = " << verifyTime << " s" << endl;
            cout << "DataSR : total deduplication query time = " << dedupTime << " s" << endl;
#endif
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
#if SYSTEM_DEBUG_FLAG == 1
            cerr << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
#endif
            switch (netBody.messageType) {
            case CLIENT_EXIT: {
                netBody.messageType = SERVER_JOB_DONE_EXIT_PERMIT;
                netBody.dataSize = 0;
                sendSize = sizeof(NetworkHeadStruct_t);
                memset(sendBuffer, 0, NETWORK_MESSAGE_DATA_SIZE);
                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
#if SYSTEM_BREAK_DOWN == 1
                cout << "DataSR : total pow Verify time = " << verifyTime << " s" << endl;
                cout << "DataSR : total deduplication query time = " << dedupTime << " s" << endl;
#endif
#if SYSTEM_LOG_FLAG == 1
                cerr << "DataSR : pow thread recv exit flag, exit now" << endl;
#endif
                return;
            }
            case POW_THREAD_DOWNLOAD: {
#if SYSTEM_LOG_FLAG == 1
                cerr << "DataSR : client download data, pow thread exit now" << endl;
#endif
                return;
            }
            case CLIENT_GET_KEY_SERVER_SK: {
                netBody.messageType = SUCCESS;
                netBody.dataSize = SYSTEM_CIPHER_SIZE;
                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), keyExchangeKey_, SYSTEM_CIPHER_SIZE);
                sendSize = sizeof(NetworkHeadStruct_t) + SYSTEM_CIPHER_SIZE;
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                enclaveSession* newSession = new enclaveSession;
                memset(newSession->sk, 0, 16);
                newSession->enclaveTrusted = true;
                powServerObj_->sessions.insert(make_pair(clientID, newSession));
                currentSession = newSession;
                break;
            }
            case SGX_SIGNED_HASH: {
                u_char clientMac[16];
                memcpy(clientMac, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(uint8_t) * 16);
                int signedHashSize = netBody.dataSize - sizeof(uint8_t) * 16;
                int signedHashNumber = signedHashSize / SYSTEM_CIPHER_SIZE;
                u_char hashList[signedHashSize];
                memcpy(hashList, recvBuffer + sizeof(NetworkHeadStruct_t) + sizeof(uint8_t) * 16, signedHashSize);
                if (currentSession == nullptr || !currentSession->enclaveTrusted) {
                    cerr << "PowServer : client not trusted yet" << endl;
                    netBody.messageType = ERROR_CLOSE;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartDataSR, NULL);
#endif
#if MULTI_CLIENT_UPLOAD_TEST == 1
                    mutexCrypto_.lock();
#endif
                    bool powVerifyStatus = powServerObj_->process_signedHash(powServerObj_->sessions.at(clientID), clientMac, hashList, signedHashNumber);
#if MULTI_CLIENT_UPLOAD_TEST == 1
                    mutexCrypto_.unlock();
#endif
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendDataSR, NULL);
                    diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                    second = diff / 1000000.0;
                    verifyTime += second;
#endif
                    if (powVerifyStatus) {
                        bool requiredChunkTemp[signedHashNumber];
                        int requiredChunkNumber = 0;
#if SYSTEM_BREAK_DOWN == 1
                        gettimeofday(&timestartDataSR, NULL);
#endif
                        bool dedupQueryStatus = dedupCoreObj_->dedupByHash(hashList, signedHashNumber, requiredChunkTemp, requiredChunkNumber);
#if SYSTEM_BREAK_DOWN == 1
                        gettimeofday(&timeendDataSR, NULL);
                        diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                        second = diff / 1000000.0;
                        dedupTime += second;
#endif
                        if (dedupQueryStatus) {
                            netBody.messageType = SUCCESS;
                            netBody.dataSize = sizeof(int) + sizeof(bool) * signedHashNumber;
                            memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                            memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &requiredChunkNumber, sizeof(int));
                            memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + sizeof(int), requiredChunkTemp, signedHashNumber * sizeof(bool));
                            sendSize = sizeof(NetworkHeadStruct_t) + netBody.dataSize;
                        } else {
                            cerr << "DedupCore : recv sgx signed hash success, dedup stage report error" << endl;
                            netBody.messageType = ERROR_RESEND;
                            netBody.dataSize = 0;
                            memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                            sendSize = sizeof(NetworkHeadStruct_t);
                        }
                    } else {
                        netBody.messageType = ERROR_RESEND;
                        netBody.dataSize = 0;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
                    }
                    powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                    break;
                }
            }
            default:
                continue;
            }
        }
    }
    return;
}