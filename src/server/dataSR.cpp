
#include <dataSR.hpp>
#include <sys/times.h>

struct timeval timestartDataSR;
struct timeval timeendDataSR;

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

DataSR::DataSR(StorageCore* storageObj, DedupCore* dedupCoreObj, powServer* powServerObj)
{
    restoreChunkBatchSize = config.getSendChunkBatchSize();
    storageObj_ = storageObj;
    dedupCoreObj_ = dedupCoreObj;
    powServerObj_ = powServerObj;
}

void DataSR::run(Socket socket)
{
    int recvSize = 0;
    int sendSize = 0;
    u_char recvBuffer[NETWORK_MESSAGE_DATA_SIZE];
    u_char sendBuffer[NETWORK_MESSAGE_DATA_SIZE];
    // double totalSaveChunkTime = 0;
    uint32_t startID = 0;
    uint32_t endID = startID + restoreChunkBatchSize;
    Recipe_t restoredFileRecipe;
    uint32_t totalRestoredChunkNumber = 0;
    while (true) {

        if (!socket.Recv(recvBuffer, recvSize)) {
            cerr << "DataSR : client closed socket connect, fd = " << socket.fd_ << " Thread exit now" << endl;
            // printf("server save all chunk time is %lf s\n", totalSaveChunkTime);
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
            cerr << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
            switch (netBody.messageType) {
            case CLIENT_EXIT: {
                return;
            }
            case CLIENT_UPLOAD_CHUNK: {
                if (!storageObj_->saveChunks(netBody, (char*)recvBuffer + sizeof(NetworkHeadStruct_t))) {
                    cerr << "DedupCore : dedup stage 2 report error" << endl;
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
                cerr << "StorageCore : recv Recipe from client" << endl;

                if (!storageObj_->checkRecipeStatus(tempRecipeHead, tempRecipeEntryList)) {
                    cerr << "StorageCore : verify Recipe fail, send resend flag" << endl;
                    netBody.messageType = ERROR_RESEND;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    cerr << "StorageCore : verify Recipe succes" << endl;
                    netBody.messageType = SUCCESS;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                }
                // socket.Send(sendBuffer, sendSize);
                break;
            }
            case CLIENT_DOWNLOAD_FILEHEAD: {

                if (storageObj_->restoreRecipeHead((char*)recvBuffer + sizeof(NetworkHeadStruct_t), restoredFileRecipe)) {
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
                socket.Send(sendBuffer, sendSize);
                break;
            }
            case CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE: {

                while (totalRestoredChunkNumber != restoredFileRecipe.fileRecipeHead.totalChunkNumber) {
                    ChunkList_t restoredChunkList;
                    if (storageObj_->restoreRecipeAndChunk((char*)recvBuffer + sizeof(NetworkHeadStruct_t), startID, endID, restoredChunkList)) {
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
                    }
                    socket.Send(sendBuffer, sendSize);
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

void DataSR::runPow(Socket socket)
{
    sgx_msg01_t msg01;
    sgx_ra_msg2_t msg2;
    sgx_ra_msg3_t* msg3;
    ra_msg4_t msg4;
    int recvSize = 0;
    int sendSize = 0;
    u_char recvBuffer[NETWORK_MESSAGE_DATA_SIZE];
    u_char sendBuffer[NETWORK_MESSAGE_DATA_SIZE];
    while (true) {

        if (!socket.Recv(recvBuffer, recvSize)) {
            cerr << "DataSR : client closed socket connect, fd = " << socket.fd_ << " Thread exit now" << endl;
            powServerObj_->closeSession(socket.fd_);
            return;
        } else {
            NetworkHeadStruct_t netBody;
            memcpy(&netBody, recvBuffer, sizeof(NetworkHeadStruct_t));
            cerr << "DataSR : recv message type " << netBody.messageType << ", message size = " << netBody.dataSize << endl;
            switch (netBody.messageType) {
            case CLIENT_EXIT: {
                return;
            }
            case SGX_RA_MSG01: {
                memcpy(&msg01.msg0_extended_epid_group_id, recvBuffer + sizeof(NetworkHeadStruct_t), sizeof(msg01.msg0_extended_epid_group_id));
                memcpy(&msg01.msg1, recvBuffer + sizeof(NetworkHeadStruct_t) + sizeof(msg01.msg0_extended_epid_group_id), sizeof(sgx_ra_msg1_t));
                if (!powServerObj_->process_msg01(socket.fd_, msg01, msg2)) {
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
                socket.Send(sendBuffer, sendSize);
                break;
            }
            case SGX_RA_MSG3: {
                msg3 = (sgx_ra_msg3_t*)new char[netBody.dataSize];
                memcpy(msg3, recvBuffer + sizeof(NetworkHeadStruct_t), netBody.dataSize);

                if (powServerObj_->sessions.find(socket.fd_) == powServerObj_->sessions.end()) {
                    cerr << "PowServer : client had not send msg01 before" << endl;
                    netBody.messageType = ERROR_CLOSE;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    // cerr << "PowServer : msg.datasize = " << netBody.dataSize << " sgx_ra_msg3_t size = " << sizeof(sgx_ra_msg3_t) << endl;
                    if (powServerObj_->process_msg3(powServerObj_->sessions[socket.fd_], msg3, msg4, netBody.dataSize - sizeof(sgx_ra_msg3_t))) {
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
                socket.Send(sendBuffer, sendSize);
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
                if (powServerObj_->sessions.find(socket.fd_) == powServerObj_->sessions.end()) {
                    cerr << "PowServer : client not trusted yet" << endl;
                    netBody.messageType = ERROR_CLOSE;
                    netBody.dataSize = 0;
                    memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                    sendSize = sizeof(NetworkHeadStruct_t);
                } else {
                    if (!powServerObj_->sessions.at(socket.fd_)->enclaveTrusted) {
                        cerr << "PowServer : client not trusted yet" << endl;
                        netBody.messageType = ERROR_CLOSE;
                        netBody.dataSize = 0;
                        memcpy(sendBuffer, &netBody, sizeof(NetworkHeadStruct_t));
                        sendSize = sizeof(NetworkHeadStruct_t);
                    } else {
                        if (powServerObj_->process_signedHash(powServerObj_->sessions.at(socket.fd_), clientReq)) {

                            RequiredChunk_t requiredChunkTemp;
                            // cerr << "Client req signature = " << endl;
                            // PRINT_BYTE_ARRAY_DATA_SR(stderr, clientReq.signature_, 16);
                            // cerr << "Client req hash size = " << clientReq.hash_.size() << endl;
                            if (dedupCoreObj_->dedupByHash(clientReq, requiredChunkTemp)) {
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
                socket.Send(sendBuffer, sendSize);
                break;
            }
            default:
                continue;
            }
        }
    }
    return;
}
