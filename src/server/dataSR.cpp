
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
    restoreChunkBatchSize = config.getSendChunkBatchSize();
    storageObj_ = storageObj;
    dedupCoreObj_ = dedupCoreObj;
    powServerObj_ = powServerObj;
    keyExchangeKeySetFlag_ = false;
    powSecurityChannel_ = powSecurityChannelTemp;
    dataSecurityChannel_ = dataSecurityChannelTemp;
    keyRegressionCurrentTimes_ = config.getKeyRegressionMaxTimes();
#if SYSTEM_DEBUG_FLAG == 1
    cout << " DataSR : key regression current count = " << keyRegressionCurrentTimes_ << endl;
#endif
    // memcpy(keyExchangeKey_, keyExchangeKey, 16);
}

void DataSR::runData(SSL* sslConnection)
{
    int recvSize = 0;
    int sendSize = 0;
    char recvBuffer[NETWORK_MESSAGE_DATA_SIZE];
    char sendBuffer[NETWORK_MESSAGE_DATA_SIZE];
    // double totalSaveChunkTime = 0;
    uint32_t startID = 0;
    uint32_t endID = 0;
    Recipe_t restoredFileRecipe;
    uint32_t totalRestoredChunkNumber = 0;
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_WHOLE_RECIPE_FILE
    RecipeList_t restoredRecipeList;
    uint64_t recipeSize = 0;
#endif
#if SYSTEM_BREAK_DOWN == 1
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
            cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
            cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
#endif
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
#if SYSTEM_DEBUG_FLAG == 1
            cout << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
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
                cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
#endif
                cerr << "DataSR : data thread recv exit flag, thread exit now" << endl;
                return;
            }
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_ONLY_KEY_RECIPE_FILE
            case CLIENT_UPLOAD_CHUNK: {
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartDataSR, NULL);
#endif
                bool saveChunkStatus = storageObj_->saveChunks(netBody, (char*)recvBuffer + sizeof(NetworkHeadStruct_t));
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendDataSR, NULL);
                diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                second = diff / 1000000.0;
                saveChunkTime += second;
#endif
                if (!saveChunkStatus) {
                    cerr << "DedupCore : save chunks report error" << endl;
#if SYSTEM_BREAK_DOWN == 1
                    cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                    cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
#endif
                    return;
                }
                break;
            }
            case CLIENT_UPLOAD_RECIPE: {
                Recipe_t tempRecipeHead;
                memcpy(&tempRecipeHead, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(Recipe_t));
                int currentRecipeEntryNumber = (netBody.dataSize - sizeof(Recipe_t)) / sizeof(RecipeEntry_t);
                RecipeList_t tempRecipeEntryList;
                for (int i = 0; i < currentRecipeEntryNumber; i++) {
                    RecipeEntry_t tempRecipeEntry;
                    memcpy(&tempRecipeEntry, recvBuffer + sizeof(NetworkHeadStruct_t) + sizeof(Recipe_t) + i * sizeof(RecipeEntry_t), sizeof(RecipeEntry_t));
                    tempRecipeEntryList.push_back(tempRecipeEntry);
                }
                cout << "StorageCore : recv Recipe from client " << netBody.clientID << endl;
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartDataSR, NULL);
#endif
                bool saveRecipeStatus = storageObj_->checkRecipeStatus(tempRecipeHead, tempRecipeEntryList);
#if SYSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendDataSR, NULL);
                diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                second = diff / 1000000.0;
                saveRecipeTime += second;
#endif
                if (saveRecipeStatus) {
                    cout << "StorageCore : verify Recipe succes" << endl;
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    cerr << "StorageCore : verify Recipe fail, send resend flag" << endl;
                    netBody.messageType = ERROR_RESEND;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                }
                dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                break;
            }
            case CLIENT_DOWNLOAD_FILEHEAD: {

                if (storageObj_->restoreRecipeHead((char*)recvBuffer + sizeof(NetworkHeadStruct_t), restoredFileRecipe)) {
                    cout << "StorageCore : restore file size = " << restoredFileRecipe.fileRecipeHead.fileSize << " chunk number = " << restoredFileRecipe.fileRecipeHead.totalChunkNumber << endl;
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = sizeof(Recipe_t);
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &restoredFileRecipe, sizeof(Recipe_t));
                    sendSize = sizeof(NetworkHeadStruct_t) + sizeof(Recipe_t);
                } else {
                    netBody.messageType = ERROR_FILE_NOT_EXIST;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                }
                dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                break;
            }
            case CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE: {
                if (restoredFileRecipe.fileRecipeHead.totalChunkNumber < config.getSendChunkBatchSize()) {
                    endID = restoredFileRecipe.fileRecipeHead.totalChunkNumber - 1;
                }
                uint32_t restoreRecipeListSize = sizeof(char) * sizeof(RecipeEntry_t) * restoredFileRecipe.fileRecipeHead.totalChunkNumber;
                char* recipeList = (char*)malloc(restoreRecipeListSize);
                bool restoreRecipeListStatus = storageObj_->restoreRecipeList((char*)recvBuffer + sizeof(NetworkHeadStruct_t), recipeList, restoreRecipeListSize);
                if (restoreRecipeListStatus) {
                    cout << "DataSR : restore recipes list done" << endl;
                    while (totalRestoredChunkNumber != restoredFileRecipe.fileRecipeHead.totalChunkNumber) {
                        ChunkList_t restoredChunkList;
                        if (storageObj_->restoreChunks(recipeList, restoreRecipeListSize, startID, endID, restoredChunkList)) {
                            cout << "DataSR : restore chunks from " << startID << " to " << endID << " done" << endl;
                            netBody.messageType = SUCCESS;
                            int currentChunkNumber = restoredChunkList.size();
                            int totalSendSize = sizeof(int);
                            memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &currentChunkNumber, sizeof(int));
                            for (int i = 0; i < currentChunkNumber; i++) {
                                memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].ID, sizeof(uint32_t));
                                totalSendSize += sizeof(uint32_t);
                                memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].logicDataSize, sizeof(int));
                                totalSendSize += sizeof(int);
                                memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].logicData, restoredChunkList[i].logicDataSize);
                                totalSendSize += restoredChunkList[i].logicDataSize;
                                memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                                totalSendSize += CHUNK_ENCRYPT_KEY_SIZE;
                            }
                            netBody.dataSize = totalSendSize;
                            memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                            sendSize = sizeof(NetworkHeadStruct_t) + totalSendSize;
                            totalRestoredChunkNumber += restoredChunkList.size();
                            startID = endID;
                            if (restoredFileRecipe.fileRecipeHead.totalChunkNumber - totalRestoredChunkNumber < restoreChunkBatchSize) {
                                endID += restoredFileRecipe.fileRecipeHead.totalChunkNumber - totalRestoredChunkNumber;
                            } else {
                                endID += config.getSendChunkBatchSize();
                            }
                        } else {
                            netBody.dataSize = 0;
                            netBody.messageType = ERROR_CHUNK_NOT_EXIST;
                            memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                            sendSize = sizeof(NetworkHeadStruct_t);
                            dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                            free(recipeList);
                            return;
                        }
                        dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                    }
                    free(recipeList);
                } else {
                    cerr << "DataSR : error restore target file recipes" << endl;
                    free(recipeList);
                    return;
                }
                break;
            }
#elif RECIPE_MANAGEMENT_METHOD == ENCRYPT_WHOLE_RECIPE_FILE
            case CLIENT_UPLOAD_CHUNK: {
#if SYSTEM_BREAK_DOWN == 1
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
                    cerr << "DedupCore : dedup stage 2 report error" << endl;
                    return;
                }
                break;
            }
            case CLIENT_UPLOAD_ENCRYPTED_RECIPE: {
                int recipeListSize = netBody.dataSize;
                cout << "DataSR : recv file recipe size = " << recipeListSize << endl;
                char* recipeListBuffer = (char*)malloc(sizeof(char) * recipeListSize + sizeof(NetworkHeadStruct_t));
                if (!dataSecurityChannel_->recv(sslConnection, recipeListBuffer, recvSize)) {
                    cout << "DataSR : client closed socket connect, recipe store failed, Thread exit now" << endl;
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
                // cout << "DataSR : current recipe size = " << recipeSize << ", toatl chunk number = " << restoredFileRecipe.fileRecipeHead.totalChunkNumber << endl;
                uint64_t decryptedRecipeListSize = 0;
                memcpy(&decryptedRecipeListSize, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(uint64_t));
                // cout << "DataSR : process recipe list size = " << decryptedRecipeListSize << endl;
                char* recvDecryptedRecipeBuffer = (char*)malloc(sizeof(char) * decryptedRecipeListSize + sizeof(NetworkHeadStruct_t));
                if (dataSecurityChannel_->recv(sslConnection, recvDecryptedRecipeBuffer, recvSize)) {
                    NetworkHeadStruct_t tempHeader;
                    memcpy(&tempHeader, recvDecryptedRecipeBuffer, sizeof(NetworkHeadStruct_t));
                    // cout << "DataSR : CLIENT_UPLOAD_DECRYPTED_RECIPE, recv message type " << tempHeader.messageType << ", message size = " << tempHeader.dataSize << endl;
                } else {
                    cerr << "DataSR : recv decrypted file recipe error " << endl;
                }
                int restoreChunkNumber = restoredFileRecipe.fileRecipeHead.totalChunkNumber;
                // cout << "DataSR : target restore chunk number = " << restoreChunkNumber << endl;
                // memcpy(&restoredFileRecipe, recvDecryptedRecipeBuffer + sizeof(NetworkHeadStruct_t), sizeof(Recipe_t));
                for (int i = 0; i < restoreChunkNumber; i++) {
                    RecipeEntry_t newRecipeEntry;
                    memcpy(&newRecipeEntry, recvDecryptedRecipeBuffer + sizeof(NetworkHeadStruct_t) + i * sizeof(RecipeEntry_t), sizeof(RecipeEntry_t));
                    // cout << "DataSR : recv chunk id = " << newRecipeEntry.chunkID << ", chunk size = " << newRecipeEntry.chunkSize << endl;
                    restoredRecipeList.push_back(newRecipeEntry);
                }
                free(recvDecryptedRecipeBuffer);
                cout << "DataSR : process recipe list done" << endl;
                break;
            }
            case CLIENT_DOWNLOAD_ENCRYPTED_RECIPE: {

                if (storageObj_->restoreRecipesSize((char*)recvBuffer + sizeof(NetworkHeadStruct_t), recipeSize)) {
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = recipeSize;
                    sendSize = sizeof(NetworkHeadStruct_t);
                    memset(sendBuffer, 0, NETWORK_MESSAGE_DATA_SIZE);
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                    u_char* recipeBuffer = (u_char*)malloc(sizeof(u_char) * recipeSize);
                    storageObj_->restoreRecipes((char*)recvBuffer + sizeof(NetworkHeadStruct_t), recipeBuffer, recipeSize);
                    char* sendRecipeBuffer = (char*)malloc(sizeof(char) * recipeSize + sizeof(NetworkHeadStruct_t));
                    memcpy(sendRecipeBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    memcpy(sendRecipeBuffer + sizeof(NetworkHeadStruct_t), recipeBuffer, recipeSize);
                    sendSize = sizeof(NetworkHeadStruct_t) + recipeSize;
                    dataSecurityChannel_->send(sslConnection, sendRecipeBuffer, sendSize);
                    memcpy(&restoredFileRecipe, recipeBuffer, sizeof(Recipe_t));
#if SYSTEM_DEBUG_FLAG == 1
                    cout << "StorageCore : send encrypted recipe list done, file size = " << restoredFileRecipe.fileRecipeHead.fileSize << ", total chunk number = " << restoredFileRecipe.fileRecipeHead.totalChunkNumber << endl;
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
                cout << "DataSR : start retrive chunks " << endl;
                if (restoredFileRecipe.fileRecipeHead.totalChunkNumber < config.getSendChunkBatchSize()) {
                    endID = restoredFileRecipe.fileRecipeHead.totalChunkNumber - 1;
                }
                while (totalRestoredChunkNumber != restoredFileRecipe.fileRecipeHead.totalChunkNumber) {
                    ChunkList_t restoredChunkList;
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timestartDataSR, NULL);
#endif
                    if (storageObj_->restoreRecipeAndChunk(restoredRecipeList, startID, endID, restoredChunkList)) {
                        netBody.messageType = SUCCESS;
                        int currentChunkNumber = restoredChunkList.size();
                        int totalSendSize = sizeof(int);
                        memset(sendBuffer, 0, NETWORK_MESSAGE_DATA_SIZE);
                        memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &currentChunkNumber, sizeof(int));
                        for (int i = 0; i < currentChunkNumber; i++) {
                            memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].ID, sizeof(uint32_t));
                            totalSendSize += sizeof(uint32_t);
                            memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].logicDataSize, sizeof(int));
                            totalSendSize += sizeof(int);
                            memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + totalSendSize, &restoredChunkList[i].logicData, restoredChunkList[i].logicDataSize);
                            totalSendSize += restoredChunkList[i].logicDataSize;
                        }
                        netBody.dataSize = totalSendSize;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t) + totalSendSize;
                        totalRestoredChunkNumber += restoredChunkList.size();
                        startID = endID;
                        if (restoredFileRecipe.fileRecipeHead.totalChunkNumber - totalRestoredChunkNumber < restoreChunkBatchSize) {
                            endID += restoredFileRecipe.fileRecipeHead.totalChunkNumber - totalRestoredChunkNumber;
                        } else {
                            endID += config.getSendChunkBatchSize();
                        }
                    } else {
                        netBody.dataSize = 0;
                        netBody.messageType = ERROR_CHUNK_NOT_EXIST;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
                        return;
                    }
#if SYSTEM_BREAK_DOWN == 1
                    gettimeofday(&timeendDataSR, NULL);
                    diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                    second = diff / 1000000.0;
                    restoreChunkTime += second;
#endif
                    dataSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                }
                break;
            }
#endif
            default:
                continue;
            }
        }
    }
    return;
}

void DataSR::runPow(SSL* sslConnection)
{
    sgx_msg01_t msg01;
    sgx_ra_msg2_t msg2;
    sgx_ra_msg3_t* msg3;
    ra_msg4_t msg4;
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
            cout << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
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
                cerr << "DataSR : pow thread recv exit flag, exit now" << endl;
                return;
            }
            case CLIENT_SET_LOGIN: {
                cerr << "DataSR : client send login message, init session" << endl;
                clientID = netBody.clientID;
                cout << "DataSR : connected client ID = " << clientID << endl;
                netBody.messageType = SUCCESS;
                netBody.dataSize = 0;
                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                sendSize = sizeof(NetworkHeadStruct_t);
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                continue;
            }
            case CLIENT_SET_LOGIN_WITH_SEAL: {
                cout << "DataSR : client send login message, loading session" << endl;
                clientID = netBody.clientID;
                cout << "DataSR : connected client ID = " << clientID << endl;
                if (powServerObj_->sessions.find(clientID) == powServerObj_->sessions.end()) {
                    cerr << "PowServer : client not trusted yet" << endl;
                    netBody.messageType = ERROR_CLOSE;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    if (!powServerObj_->sessions.at(clientID)->enclaveTrusted) {
                        cerr << "PowServer : client not trusted yet, client ID exist but not passed" << endl;
                        netBody.messageType = ERROR_CLOSE;
                        netBody.dataSize = 0;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
                    } else {
                        currentSession = powServerObj_->sessions.at(clientID);
#if SYSTEM_DEBUG_FLAG == 1
                        cout << "DataSR : client sealed login success, session key = " << endl;
                        PRINT_BYTE_ARRAY_DATA_SR(stderr, currentSession->sk, 16);
#endif
                        netBody.messageType = SUCCESS;
                        netBody.dataSize = 0;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
                    }
                }
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                continue;
            }
            case CLIENT_SET_LOGOUT: {
                cerr << "DataSR : client send logout message, clean up loged session" << endl;
                powServerObj_->closeSession(netBody.clientID);
                netBody.messageType = SUCCESS;
                netBody.dataSize = 0;
                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                sendSize = sizeof(NetworkHeadStruct_t);
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                continue;
            }
            case CLIENT_GET_KEY_SERVER_SK: {
                if (keyExchangeKeySetFlag_ == true) {
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = KEY_SERVER_SESSION_KEY_SIZE;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), keyExchangeKey_, KEY_SERVER_SESSION_KEY_SIZE);
                    sendSize = sizeof(NetworkHeadStruct_t) + KEY_SERVER_SESSION_KEY_SIZE;
                } else {
                    netBody.messageType = ERROR_CLOSE;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                }
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                break;
            }
            case SGX_RA_MSG01: {
                memcpy(&msg01.msg0_extended_epid_group_id, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(msg01.msg0_extended_epid_group_id));
                memcpy(&msg01.msg1, recvBuffer + sizeof(NetworkHeadStruct_t) + sizeof(msg01.msg0_extended_epid_group_id), sizeof(sgx_ra_msg1_t));
                if (!powServerObj_->process_msg01(clientID, msg01, msg2)) {
                    cerr << "PowServer : error process msg01" << endl;
                    netBody.messageType = ERROR_RESEND;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = sizeof(sgx_ra_msg2_t);
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &msg2, sizeof(sgx_ra_msg2_t));
                    sendSize = sizeof(NetworkHeadStruct_t) + sizeof(sgx_ra_msg2_t);
                }
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                break;
            }
            case SGX_RA_MSG3: {
                msg3 = (sgx_ra_msg3_t*)new char[netBody.dataSize];
                memcpy(msg3, recvBuffer + sizeof(NetworkHeadStruct_t), netBody.dataSize);
                if (powServerObj_->sessions.find(clientID) == powServerObj_->sessions.end()) {
                    cerr << "PowServer : client had not send msg01 before" << endl;
                    netBody.messageType = ERROR_CLOSE;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    if (powServerObj_->process_msg3(powServerObj_->sessions[clientID], msg3, msg4, netBody.dataSize - sizeof(sgx_ra_msg3_t))) {
                        netBody.messageType = SUCCESS;
                        netBody.dataSize = sizeof(ra_msg4_t);
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &msg4, sizeof(ra_msg4_t));
                        sendSize = sizeof(NetworkHeadStruct_t) + sizeof(ra_msg4_t);
                        currentSession = powServerObj_->sessions[clientID];
#if SYSTEM_DEBUG_FLAG == 1
                        cout << "PoWServer : client remote attestation passed, session key = " << endl;
                        PRINT_BYTE_ARRAY_DATA_SR(stderr, currentSession->sk, 16);
#endif
                    } else {
                        cerr << "PowServer : sgx process msg3 & get msg4 error" << endl;
                        netBody.messageType = ERROR_CLOSE;
                        netBody.dataSize = 0;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
                    }
                }
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                break;
            }
            case SGX_SIGNED_HASH: {
                u_char clientMac[16];
                memcpy(clientMac, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(uint8_t) * 16);
                int signedHashSize = netBody.dataSize - sizeof(uint8_t) * 16;
                int signedHashNumber = signedHashSize / CHUNK_HASH_SIZE;
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
                    bool powVerifyStatus = powServerObj_->process_signedHash(powServerObj_->sessions.at(clientID), clientMac, hashList, signedHashNumber);
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

void DataSR::runKeyServerSessionKeyUpdate()
{
    struct timeval timestart;
    struct timeval timeend;
#if SYSTEM_BREAK_DOWN == 1
    long diff;
    double second;
#endif
    while (true) {
        if (!keyExchangeKeySetFlag_) {
            continue;
        }
        if (keyServerSession_ != nullptr) {
            cout << "DataSR : start keyServer session key update, current time = " << endl;
            time_t timep;
            time(&timep);
            cout << asctime(gmtime(&timep));
            keyExchangeKeySetFlag_ = false;
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timestart, 0);
#endif
            u_char hashDataTemp[32];
            u_char hashResultTemp[32];
            memcpy(hashDataTemp, keyServerSession_->sk, 16);
            memcpy(hashDataTemp + 16, keyServerSession_->mk, 16);
            for (int i = 0; i < keyRegressionCurrentTimes_; i++) {
                SHA256(hashDataTemp, 32, hashResultTemp);
                memcpy(hashDataTemp, hashResultTemp, 32);
            }
            u_char finalHashBuffer[40];
            memset(finalHashBuffer, 0, 40);
            memcpy(finalHashBuffer + 8, hashResultTemp, 32);
            SHA256(finalHashBuffer, 40, hashResultTemp);
            memcpy(keyExchangeKey_, hashResultTemp, KEY_SERVER_SESSION_KEY_SIZE);
#if SYSTEM_BREAK_DOWN == 1
            gettimeofday(&timeend, 0);
            diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
            second = diff / 1000000.0;
#endif
#if SYSTEM_DEBUG_FLAG == 1
            cout << "DataSR : key server current session key = " << endl;
            PRINT_BYTE_ARRAY_DATA_SR(stderr, keyExchangeKey_, KEY_SERVER_SESSION_KEY_SIZE);
            cout << "DataSR : key server original session key = " << endl;
            PRINT_BYTE_ARRAY_DATA_SR(stderr, keyServerSession_->sk, 16);
            cout << "DataSR : key server original mac key = " << endl;
            PRINT_BYTE_ARRAY_DATA_SR(stderr, keyServerSession_->mk, 16);
#endif
            keyExchangeKeySetFlag_ = true;
            cout << "DataSR : keyServer session key update done, current regression counter = " << keyRegressionCurrentTimes_ << endl;
#if SYSTEM_BREAK_DOWN == 1
            cout << "DataSR : session key update time = " << second << " s" << endl;
#endif
            keyRegressionCurrentTimes_--;
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC_);
            xt.sec += config.getKeyRegressionIntervals();
            boost::thread::sleep(xt);
        }
    }
    return;
}

void DataSR::runKeyServerRemoteAttestation()
{
    ssl* sslRAListen = new ssl(config.getStorageServerIP(), config.getKMServerPort(), SERVERSIDE);
    cerr << "DataSR : key server ra request channel setup" << endl;
    int sendSize = sizeof(NetworkHeadStruct_t);
    char sendBuffer[sendSize];
    NetworkHeadStruct_t netHead, recvHead;
    netHead.messageType = RA_REQUEST;
    netHead.dataSize = 0;
    memcpy(sendBuffer, &netHead, sizeof(NetworkHeadStruct_t));
    while (true) {
        SSL* sslRAListenConnection = sslRAListen->sslListen().second;
        cerr << "DataSR : key server connected" << endl;
        char recvBuffer[sizeof(NetworkHeadStruct_t)];
        int recvSize;
        sslRAListen->recv(sslRAListenConnection, recvBuffer, recvSize);
        memcpy(&recvHead, recvBuffer, sizeof(NetworkHeadStruct_t));
        if (recvHead.messageType == KEY_SERVER_RA_REQUES) {
            cout << "DataSR : key server start remote attestation now, current time = " << endl;
            time_t timep;
            time(&timep);
            cout << asctime(gmtime(&timep));
            kmServer server(sslRAListen, sslRAListenConnection);
            keyServerSession_ = server.authkm();
            if (keyServerSession_ != nullptr) {
                keyExchangeKeySetFlag_ = true;
                cerr << "DataSR : keyServer enclave trusted" << endl;
                // delete sslRAListenConnection
                free(sslRAListenConnection);
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC_);
                xt.sec += config.getRASessionKeylifeSpan();
                boost::thread::sleep(xt);
                keyExchangeKeySetFlag_ = false;
                memset(keyExchangeKey_, 0, KEY_SERVER_SESSION_KEY_SIZE);
                ssl* sslRARequest = new ssl(config.getKeyServerIP(), config.getkeyServerRArequestPort(), CLIENTSIDE);
                SSL* sslRARequestConnection = sslRARequest->sslConnect().second;
                sslRARequest->send(sslRARequestConnection, sendBuffer, sendSize);
                // delete sslRARequest;
                free(sslRARequestConnection);
            } else {
                cerr << "DataSR : keyServer send wrong message, storage try again now" << endl;
                continue;
            }
        } else {
            cerr << "DataSR : keyServer enclave not trusted, storage try again now, request type = " << recvHead.messageType << endl;
            continue;
        }
    }
    return;
}
