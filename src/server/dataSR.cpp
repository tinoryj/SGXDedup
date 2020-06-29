
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
    keyExchangeKeySetFlag = false;
    powSecurityChannel_ = powSecurityChannelTemp;
    dataSecurityChannel_ = dataSecurityChannelTemp;
    keyRegressionCurrentTimes_ = config.getKeyRegressionMaxTimes();
    // memcpy(keyExchangeKey_, keyExchangeKey, 16);
}

void DataSR::run(SSL* sslConnection)
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
#if SGSTEM_BREAK_DOWN == 1
    struct timeval timestartDataSR;
    struct timeval timeendDataSR;
    double saveChunkTime = 0;
    double saveRecipeTime = 0;
    long diff;
    double second;
#endif
    while (true) {
        if (!dataSecurityChannel_->recv(sslConnection, recvBuffer, recvSize)) {
            cerr << "DataSR : client closed socket connect, thread exit now" << endl;
#if SGSTEM_BREAK_DOWN == 1
            cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
            cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
#endif
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
            // cerr << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
            switch (netBody.messageType) {
            case CLIENT_EXIT: {
#if SGSTEM_BREAK_DOWN == 1
                cout << "DataSR : total save chunk time = " << saveChunkTime << " s" << endl;
                cout << "DataSR : total save recipe time = " << saveRecipeTime << " s" << endl;
#endif
                cerr << "DataSR : data thread recv exit flag, thread exit now";
                return;
            }
            case CLIENT_UPLOAD_CHUNK: {
#if SGSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartDataSR, NULL);
#endif
                bool saveChunkStatus = storageObj_->saveChunks(netBody, (char*)recvBuffer + sizeof(NetworkHeadStruct_t));
#if SGSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendDataSR, NULL);
                diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                second = diff / 1000000.0;
                saveChunkTime += second;
#endif
                if (!saveChunkStatus) {
                    cerr << "DedupCore : save chunks report error" << endl;
#if SGSTEM_BREAK_DOWN == 1
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
                cerr << "StorageCore : recv Recipe from client " << netBody.clientID << endl;
#if SGSTEM_BREAK_DOWN == 1
                gettimeofday(&timestartDataSR, NULL);
#endif
                bool saveRecipeStatus = storageObj_->checkRecipeStatus(tempRecipeHead, tempRecipeEntryList);
#if SGSTEM_BREAK_DOWN == 1
                gettimeofday(&timeendDataSR, NULL);
                diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                second = diff / 1000000.0;
                saveRecipeTime += second;
#endif
                if (saveRecipeStatus) {
                    cerr << "StorageCore : verify Recipe succes" << endl;
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
                    cerr << "StorageCore : restore file size = " << restoredFileRecipe.fileRecipeHead.fileSize << " chunk number = " << restoredFileRecipe.fileRecipeHead.totalChunkNumber << endl;
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
                    cerr << "DataSR : restore recipes list done" << endl;
                    while (totalRestoredChunkNumber != restoredFileRecipe.fileRecipeHead.totalChunkNumber) {
                        ChunkList_t restoredChunkList;
                        if (storageObj_->restoreChunks(recipeList, restoreRecipeListSize, startID, endID, restoredChunkList)) {
                            cerr << "DataSR : restore chunks from " << startID << " to " << endID << " done" << endl;
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
    int clientID;
#if SGSTEM_BREAK_DOWN == 1
    struct timeval timestartDataSR;
    struct timeval timeendDataSR;
    double verifyTime = 0;
    double dedupTime = 0;
    long diff;
    double second;
#endif
    while (true) {

        if (!powSecurityChannel_->recv(sslConnection, recvBuffer, recvSize)) {
            cerr << "DataSR : client closed socket connect, Client ID = " << clientID << " Thread exit now" << endl;
#if SGSTEM_BREAK_DOWN == 1
            cout << "DataSR : total pow Verify time = " << verifyTime << " s" << endl;
            cout << "DataSR : total deduplication query time = " << dedupTime << " s" << endl;
#endif
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
            // cerr << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
            switch (netBody.messageType) {
            case CLIENT_EXIT: {
#if SGSTEM_BREAK_DOWN == 1
                cout << "DataSR : total pow Verify time = " << verifyTime << " s" << endl;
                cout << "DataSR : total deduplication query time = " << dedupTime << " s" << endl;
#endif
                cerr << "DataSR : pow thread recv exit flag, exit now" << endl;
                return;
            }
            case CLIENT_SET_LOGIN: {
                cerr << "DataSR : client send login message, init session" << endl;
                clientID = netBody.clientID;
                cerr << "DataSR : set client ID = " << clientID << endl;
                netBody.messageType = SUCCESS;
                netBody.dataSize = 0;
                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                sendSize = sizeof(NetworkHeadStruct_t);
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
                if (keyExchangeKeySetFlag == true) {
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
                    // cerr << "PowServer : msg.datasize = " << netBody.dataSize << " sgx_ra_msg3_t size = " << sizeof(sgx_ra_msg3_t) << endl;
                    if (powServerObj_->process_msg3(powServerObj_->sessions[clientID], msg3, msg4, netBody.dataSize - sizeof(sgx_ra_msg3_t))) {
                        netBody.messageType = SUCCESS;
                        netBody.dataSize = sizeof(ra_msg4_t);
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &msg4, sizeof(ra_msg4_t));
                        sendSize = sizeof(NetworkHeadStruct_t) + sizeof(ra_msg4_t);
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
                powSignedHash_t clientReq;
                int signedHashNumber = (netBody.dataSize - sizeof(uint8_t) * 16) / CHUNK_HASH_SIZE;
                memcpy(clientReq.signature_, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(uint8_t) * 16);
                for (int i = 0; i < signedHashNumber; i++) {
                    string hash;
                    hash.resize(CHUNK_HASH_SIZE);
                    memcpy(&hash[0], recvBuffer + sizeof(NetworkHeadStruct_t) + sizeof(uint8_t) * 16 + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
                    clientReq.hash_.push_back(hash);
                }
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
#if SGSTEM_BREAK_DOWN == 1
                        gettimeofday(&timestartDataSR, NULL);
#endif
                        bool powVerifyStatus = powServerObj_->process_signedHash(powServerObj_->sessions.at(clientID), clientReq);
#if SGSTEM_BREAK_DOWN == 1
                        gettimeofday(&timeendDataSR, NULL);
                        diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                        second = diff / 1000000.0;
                        verifyTime += second;
#endif
                        if (powVerifyStatus) {
                            RequiredChunk_t requiredChunkTemp;
#if SGSTEM_BREAK_DOWN == 1
                            gettimeofday(&timestartDataSR, NULL);
#endif
                            bool dedupQueryStatus = dedupCoreObj_->dedupByHash(clientReq, requiredChunkTemp);
#if SGSTEM_BREAK_DOWN == 1
                            gettimeofday(&timeendDataSR, NULL);
                            diff = 1000000 * (timeendDataSR.tv_sec - timestartDataSR.tv_sec) + timeendDataSR.tv_usec - timestartDataSR.tv_usec;
                            second = diff / 1000000.0;
                            dedupTime += second;
#endif
                            if (dedupQueryStatus) {
                                netBody.messageType = SUCCESS;
                                netBody.dataSize = sizeof(int) + requiredChunkTemp.size() * sizeof(uint32_t);
                                memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                                int requiredChunkNumber = requiredChunkTemp.size();
                                memcpy(sendBuffer + sizeof(NetworkHeadStruct_t), &requiredChunkNumber, sizeof(int));
                                for (int i = 0; i < requiredChunkNumber; i++) {
                                    memcpy(sendBuffer + sizeof(NetworkHeadStruct_t) + sizeof(int) + i * sizeof(uint32_t), &requiredChunkTemp[i], sizeof(uint32_t));
                                }
                                sendSize = sizeof(NetworkHeadStruct_t) + sizeof(int) + requiredChunkNumber * sizeof(uint32_t);
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
                    }
                }
                powSecurityChannel_->send(sslConnection, sendBuffer, sendSize);
                break;
            }
            default:
                continue;
            }
        }
    }
    return;
}

void DataSR::runKeyServerRA()
{
    ssl* sslRAListen = new ssl(config.getStorageServerIP(), config.getKMServerPort(), SERVERSIDE);
    cerr << "DataSR : key server ra request channel setup" << endl;
    int sendSize = sizeof(NetworkHeadStruct_t);
    char sendBuffer[sendSize];
    NetworkHeadStruct_t netHead, recvHead;
    netHead.messageType = RA_REQUEST;
    netHead.dataSize = 0;
    memcpy(sendBuffer, &netHead, sizeof(NetworkHeadStruct_t));
    powSession* session;
    while (true) {
    errorRetry:
        SSL* sslRAListenConnection = sslRAListen->sslListen().second;
        cerr << "DataSR : key server connected" << endl;
        char recvBuffer[sizeof(NetworkHeadStruct_t)];
        int recvSize;
        sslRAListen->recv(sslRAListenConnection, recvBuffer, recvSize);
        memcpy(&recvHead, recvBuffer, sizeof(NetworkHeadStruct_t));
        if (recvHead.messageType == KEY_SERVER_SESSION_KEY_UPDATE) {
            cerr << "DataSR : key server start session key update now" << endl;
            keyRegressionCurrentTimes_--;
            char currentSessionKey[KEY_SERVER_SESSION_KEY_SIZE];
            memcpy(currentSessionKey, session->sk, 16);
            memcpy(currentSessionKey + 16, session->mk, 16);
            uint8_t* hashDataTemp = (uint8_t*)malloc(32);
            uint8_t* hashResultTemp = (uint8_t*)malloc(32);
            memcpy(hashDataTemp, &currentSessionKey, 32);
            for (int i = 0; i < keyRegressionCurrentTimes_; i++) {
                SHA256(hashDataTemp, 32, hashResultTemp);
                memcpy(hashDataTemp, hashResultTemp, 32);
            }
            memcpy(currentSessionKey, hashResultTemp, 32);
            memcpy(keyExchangeKey_, currentSessionKey, KEY_SERVER_SESSION_KEY_SIZE);
            keyExchangeKeySetFlag = true;
            free(sslRAListenConnection);
        } else if (recvHead.messageType == KEY_SERVER_RA_REQUES) {
            cerr << "DataSR : key server start remote attestation now" << endl;
            kmServer server(sslRAListen, sslRAListenConnection);
            session = server.authkm();
            if (session != nullptr) {
                // memcpy(keyExchangeKey_, keyExchangeKey, 16);
                char currentSessionKey[KEY_SERVER_SESSION_KEY_SIZE];
                memcpy(currentSessionKey, session->sk, 16);
                memcpy(currentSessionKey + 16, session->mk, 16);
                uint8_t* hashDataTemp = (uint8_t*)malloc(32);
                uint8_t* hashResultTemp = (uint8_t*)malloc(32);
                memcpy(hashDataTemp, &currentSessionKey, 32);
                for (int i = 0; i < keyRegressionCurrentTimes_; i++) {
                    SHA256(hashDataTemp, 32, hashResultTemp);
                    memcpy(hashDataTemp, hashResultTemp, 32);
                }
                memcpy(currentSessionKey, hashResultTemp, 32);
                memcpy(keyExchangeKey_, currentSessionKey, KEY_SERVER_SESSION_KEY_SIZE);

                PRINT_BYTE_ARRAY_DATA_SR(stderr, keyExchangeKey_, KEY_SERVER_SESSION_KEY_SIZE);
                keyExchangeKeySetFlag = true;
                cerr << "DataSR : keyServer enclave trusted" << endl;
                delete session;
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC_);
                xt.sec += config.getRASessionKeylifeSpan();
                boost::thread::sleep(xt);
                keyExchangeKeySetFlag = false;
                memset(keyExchangeKey_, 0, KEY_SERVER_SESSION_KEY_SIZE);
                ssl* sslRARequest = new ssl(config.getKeyServerIP(), config.getkeyServerRArequestPort(), CLIENTSIDE);
                SSL* sslRARequestConnection = sslRARequest->sslConnect().second;
                sslRARequest->send(sslRARequestConnection, sendBuffer, sendSize);
                // delete sslRARequest;
                free(sslRARequestConnection);
            } else {
                cerr << "DataSR : keyServer send wrong message, storage try again now" << endl;
                goto errorRetry;
            }
        } else {
            // delete session;
            cerr << "DataSR : keyServer enclave not trusted, storage try again now, request type = " << recvHead.messageType << endl;
            goto errorRetry;
        }
    }
    return;
}
