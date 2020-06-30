#include "keyServer.hpp"
#include <fcntl.h>
#include <sys/time.h>
#if KEY_GEN_SERVER_MLE_NO_OPRF == 1
#include <openssl/sha.h>
#endif
extern Configure config;

void PRINT_BYTE_ARRAY_KEY_SERVER(
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

keyServer::keyServer(ssl* keyServerSecurityChannelTemp)
{
    rsa_ = RSA_new();
    key_ = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(key_, &rsa_, NULL, passwd);
#if OPENSSL_V_1_0_2 == 1

    keyD_ = rsa_->d;
    keyN_ = rsa_->n;
#else
    RSA_get0_key(rsa_, &keyN_, nullptr, &keyD_);
#endif
    u_char keydBuffer[4096];
    int lenKeyd;
    lenKeyd = BN_bn2bin(keyD_, keydBuffer);
    string keyd((char*)keydBuffer, lenKeyd / 2);
    sessionKeyUpdateCount_ = config.getKeyRegressionMaxTimes();
    client = new kmClient(keyd, sessionKeyUpdateCount_);
    clientThreadCount_ = 0;
    keyGenerateCount_ = 0;
    keyGenLimitPerSessionKey_ = config.getKeyGenLimitPerSessionkeySize();
    raRequestFlag = true;
    raSetupFlag = false;
    keySecurityChannel_ = keyServerSecurityChannelTemp;
#if KEY_GEN_EPOLL_MODE == 1
    epfd_ = epoll_create(100);
    requestMQ_ = new messageQueue<KeyServerEpollMessage_t>;
    responseMQ_ = new messageQueue<KeyServerEpollMessage_t>;
    for (int i = 0; i < config.getKeyEnclaveThreadNumber(); i++) {
        perThreadKeyGenerateCount_.push_back(0);
    }
#endif
}

keyServer::~keyServer()
{
    BIO_free_all(key_);
    RSA_free(rsa_);
    delete client;
    delete keySecurityChannel_;
#if KEY_GEN_EPOLL_MODE == 1
    requestMQ_->~messageQueue();
    delete requestMQ_;
    responseMQ_->~messageQueue();
    delete responseMQ_;
#endif
}

bool keyServer::getRASetupFlag()
{
    return raSetupFlag;
}

bool keyServer::doRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection)
{
    if (!client->init(raSecurityChannel, sslConnection)) {
        cerr << "keyServer : enclave not truster" << endl;
        return false;
    } else {
        raSetupFlag = true;
        cout << "KeyServer : enclave trusted" << endl;
    }
    multiThreadCountMutex_.lock();
    keyGenerateCount_ = 0;
    multiThreadCountMutex_.unlock();
    return true;
}

void keyServer::runRAwithSPRequest()
{
    ssl* raSecurityChannelTemp = new ssl(config.getKeyServerIP(), config.getkeyServerRArequestPort(), SERVERSIDE);
    char recvBuffer[sizeof(NetworkHeadStruct_t)];
    int recvSize;
    while (true) {
        SSL* sslConnection = raSecurityChannelTemp->sslListen().second;
        raSecurityChannelTemp->recv(sslConnection, recvBuffer, recvSize);
        raRequestFlag = true;
        cout << "KeyServer : recv storage server ra request, waiting for ra now" << endl;
        free(sslConnection);
    }
}

void keyServer::runRA()
{
    while (true) {
        if (raRequestFlag) {
            while (!(clientThreadCount_ == 0))
                ;
            cout << "KeyServer : start do remote attestation to storage server" << endl;
            keyGenerateCount_ = 0;
            ssl* raSecurityChannelTemp = new ssl(config.getStorageServerIP(), config.getKMServerPort(), CLIENTSIDE);
            SSL* sslConnection = raSecurityChannelTemp->sslConnect().second;
            cout << "KeyServer : remote attestation thread connected storage server" << endl;
            int sendSize = sizeof(NetworkHeadStruct_t);
            char sendBuffer[sendSize];
            NetworkHeadStruct_t netHead;
            netHead.messageType = KEY_SERVER_RA_REQUES;
            netHead.dataSize = 0;
            memcpy(sendBuffer, &netHead, sizeof(NetworkHeadStruct_t));
            raSecurityChannelTemp->send(sslConnection, sendBuffer, sendSize);
            bool remoteAttestationStatus = doRemoteAttestation(raSecurityChannelTemp, sslConnection);
            if (remoteAttestationStatus) {
                delete raSecurityChannelTemp;
                free(sslConnection);
                raRequestFlag = false;
                cout << "KeyServer : do remote attestation to storage SP done" << endl;
            } else {
                delete raSecurityChannelTemp;
                free(sslConnection);
                cerr << "KeyServer : do remote attestation to storage SP error" << endl;
                continue;
            }
        }
    }
}

void keyServer::runSessionKeyUpdate()
{
    while (true) {
        if (keyGenerateCount_ >= keyGenLimitPerSessionKey_) {
            while (!(clientThreadCount_ == 0))
                ;
            cout << "KeyServer : start session key update with storage server" << endl;
            keyGenerateCount_ = 0;
            ssl* raSecurityChannelTemp = new ssl(config.getStorageServerIP(), config.getKMServerPort(), CLIENTSIDE);
            SSL* sslConnection = raSecurityChannelTemp->sslConnect().second;
            bool enclaveSessionKeyUpdateStatus = client->sessionKeyUpdate();
            if (enclaveSessionKeyUpdateStatus) {
                char sendBuffer[sizeof(NetworkHeadStruct_t)];
                int sendSize = sizeof(NetworkHeadStruct_t);
                NetworkHeadStruct_t requestHeader;
                requestHeader.messageType = KEY_SERVER_SESSION_KEY_UPDATE;
                memcpy(sendBuffer, &requestHeader, sizeof(requestHeader));
                raSecurityChannelTemp->send(sslConnection, sendBuffer, sendSize);
                delete raSecurityChannelTemp;
                free(sslConnection);
                cout << "KeyServer : update session key storage SP done" << endl;
            } else {
                delete raSecurityChannelTemp;
                free(sslConnection);
                cerr << "KeyServer : update session key with storage SP error" << endl;
                continue;
            }
        }
    }
}

#if KEY_GEN_SGX_CTR == 1
void keyServer::runCTRModeMaskGenerate()
{
    while (true) {
        if (raRequestFlag == false) {
            while (!(clientThreadCount_ == 0))
                ;
            cout << "KeyServer : start compute session encryption mask offline" << endl;
            auto it = clientList_.begin();
            while (it != clientList_.end()) {
                if (it->second.keyGenerateCounter > 0) {
                    client->maskGenerate(it->second.clientID, it->second.keyGenerateCounter, it->second.nonce, it->second.nonceLen);
                    cout << "KeyServer : offlien mask generate done for client " << it->first << endl;
                }
                ++it;
            }
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC_);
            xt.sec += 20;
            boost::thread::sleep(xt);
        }
    }
}
#endif

#if KEY_GEN_EPOLL_MODE == 1

bool setnonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(sock,GETFL)");
        return false;
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(sock,SETFL,opts)");
        return false;
    }
    return true;
}

void keyServer::runRecvThread()
{
    cout << "KeyServer : start epoll recv thread" << endl;
    // setnonblocking(keySecurityChannel_->listenFd);
    epoll_event event[100];
    epoll_event ev;
    ev.data.ptr = nullptr;
    ev.events = EPOLLIN;
    ev.data.fd = keySecurityChannel_->listenFd;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, keySecurityChannel_->listenFd, &ev) != 0) {
        cerr << "KeyServer : epoll init error, fd = " << keySecurityChannel_->listenFd << endl;
    }
#if KEY_GEN_SGX_CTR == 1
    u_char hash[sizeof(NetworkHeadStruct_t) + config.getKeyBatchSize() * CHUNK_HASH_SIZE];
    NetworkHeadStruct_t netHead;
#elif KEY_GEN_SGX_CFB == 1
    u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
#endif
    for (int i = 0; i < 100; i++) {
        event[i].data.ptr = nullptr;
    }
    int recvSize = 0;
    cout << "KeyServer : start epoll recv thread, listen fd = " << ev.data.fd << ", " << keySecurityChannel_->listenFd << endl;
    while (true) {
        int nfd = epoll_wait(epfd_, event, 100, -1);
        // cout << "KeyServer : epoll query start, nfd = " << nfd << ", epfd = " << epfd_ << endl;
        // for (int i = 0; i < nfd; i++) {
        //     cout << "KeyServer : nfd = " << nfd << endl;
        //     if (event[i].events == EPOLLIN | EPOLLET) {
        //         cout << "KeyServer : fd = " << event[i].data.fd << ", status = EPOLLIN" << endl;
        //     } else if (event[i].events == EPOLLERR | EPOLLET) {
        //         cout << "KeyServer : fd = " << event[i].data.fd << ", status = EPOLLERR" << endl;
        //     } else if (event[i].events == EPOLLOUT | EPOLLET) {
        //         cout << "KeyServer : fd = " << event[i].data.fd << ", status = EPOLLOUT" << endl;
        //     }
        // }
        for (int i = 0; i < nfd; i++) {
            if (event[i].data.fd == keySecurityChannel_->listenFd) {
                cout << __LINE__ << " fd = " << event[i].data.fd << endl;
                std::pair<int, SSL*> con = keySecurityChannel_->sslListen();
                sslConnectionList_.insert(con);
                KeyServerEpollMessage_t msgTemp;
                msgTemp.fd = con.first;
                ev.data.ptr = (void*)&msgTemp;
                ev.events = EPOLLIN;
                ev.data.fd = con.first;
                if (epoll_ctl(epfd_, EPOLL_CTL_ADD, con.first, &ev) != 0) {
                    cerr << "KeyServer : epoll set error, fd = " << con.first << endl;
                }
                epollSessionMutex_.lock();
                epollSession_.insert(make_pair(con.first, msgTemp));
                epollSessionMutex_.unlock();
                cout << "KeyServer : epoll recved new connection, fd = " << con.first << endl;
                continue;
            } else if (event[i].events & EPOLLIN) {
                cout << __LINE__ << " fd = " << event[i].data.fd << endl;
                // cout << "KeyServer : fd = " << event[i].data.fd << " start recv data" << endl;
                if (!keySecurityChannel_->recv(sslConnectionList_[event[i].data.fd], (char*)hash, recvSize)) {
                    ev.data.ptr = nullptr;
                    ev.data.fd = event[i].data.fd;
                    ev.events = EPOLLERR;
                    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, event[i].data.fd, &ev) != 0) {
                        cerr << "KeyServer : epoll delete error, fd = " << event[i].data.fd << endl;
                    }
                    epollSessionMutex_.lock();
                    epollSession_.erase(event[i].data.fd);
                    epollSessionMutex_.unlock();
                    cerr << "KeyServer : client closed ssl connect, fd = " << event[i].data.fd << endl;
                    continue;
                } else {
#if KEY_GEN_SGX_CTR == 1
                    memcpy(&netHead, hash, sizeof(NetworkHeadStruct_t));
                    KeyServerEpollMessage_t msgIn;
                    if (netHead.messageType == KEY_GEN_UPLOAD_CLIENT_INFO) {
                        cerr << "KeyServer : recv data type is mask info" << endl;
                        uint32_t recvedClientCounter = 0;
                        int clientID = netHead.clientID;
                        char nonce[16 - sizeof(uint32_t)];
                        memcpy(&recvedClientCounter, hash + sizeof(NetworkHeadStruct_t), sizeof(uint32_t));
                        memcpy(nonce, hash + sizeof(NetworkHeadStruct_t) + sizeof(uint32_t), 16 - sizeof(uint32_t));
                        if (recvedClientCounter != 0) {
                            if (clientList_.find(clientID) == clientList_.end()) {
                                MaskInfo_t currentMaskInfo;
                                memcpy(currentMaskInfo.nonce, nonce, 12);
                                currentMaskInfo.clientID = clientID;
                                currentMaskInfo.keyGenerateCounter = 0;
                                currentMaskInfo.currentKeyGenerateCounter = 0;
                                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
                                cout << "KeyServer : insert new client mask info into list, client ID = " << currentMaskInfo.clientID << ", current clientList_ size = " << clientList_.size() << endl;
                            } else {
                                if (clientList_.at(clientID).keyGenerateCounter == recvedClientCounter) {
                                    cout << "KeyServer : compared key generate encryption counter success" << endl;
                                    continue;
                                } else {
                                    cerr << "KeyServer : compared key generate encryption counter error, recved counter = " << recvedClientCounter << ", exist counter = " << clientList_.at(clientID).keyGenerateCounter << endl;
                                    clientList_.erase(clientID);
                                    continue;
                                }
                            }
                        } else {
                            MaskInfo_t currentMaskInfo;
                            memcpy(currentMaskInfo.nonce, nonce, 12);
                            currentMaskInfo.clientID = clientID;
                            currentMaskInfo.keyGenerateCounter = 0;
                            currentMaskInfo.currentKeyGenerateCounter = 0;
                            clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
                            cout << "KeyServer : insert new client mask info into list, client ID = " << currentMaskInfo.clientID << ", current clientList_ size = " << clientList_.size() << endl;
                        }
                    } else if (netHead.messageType == KEY_GEN_UPLOAD_CHUNK_HASH) {
                        cerr << "KeyServer : recv data type is chunk hash" << endl;
                        ev.data.ptr = (void*)&msgIn;
                        ev.data.fd = event[i].data.fd;
                        ev.events = EPOLLERR;
                        if (epoll_ctl(epfd_, EPOLL_CTL_MOD, event[i].data.fd, &ev) != 0) {
                            cerr << "KeyServer : epoll in change mode error, fd = " << event[i].data.fd << endl;
                        }
                        memcpy(msgIn.hashContent, hash + sizeof(NetworkHeadStruct_t), netHead.dataSize);
                        msgIn.length = netHead.dataSize;
                        msgIn.requestNumber = msgIn.length / CHUNK_HASH_SIZE;
                        msgIn.fd = event[i].data.fd;
                        msgIn.clientID = netHead.clientID;
                    }
#elif KEY_GEN_SGX_CFB == 1
                    KeyServerEpollMessage_t msgIn;
                    ev.data.ptr = (void*)&msgIn;
                    ev.data.fd = event[i].data.fd;
                    ev.events = EPOLLERR;
                    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, event[i].data.fd, &ev) != 0) {
                        cerr << "KeyServer : epoll in change mode error, fd = " << event[i].data.fd << endl;
                    }
                    memcpy(msgIn.hashContent, hash, recvSize);
                    msgIn.length = recvSize;
                    msgIn.requestNumber = msgIn.length / CHUNK_HASH_SIZE;
                    msgIn.fd = event[i].data.fd;
                    // msgIn.clientID = netHead.clientID;
#endif
                    cout << "KeyServer : fd = " << event[i].data.fd << " recved data size = " << msgIn.length << endl;
                    requestMQ_->push(msgIn);
                    // KeyServerEpollMessage_t tempPopMsg;
                    // requestMQ_->pop(tempPopMsg);
                    // cout << "KeyServer : fd = " << tempPopMsg.fd << " pop data size = " << tempPopMsg.length << endl;
                    // requestMQ_->push(tempPopMsg);
                    // cout << "KeyServer : fd = " << tempPopMsg.fd << " push data size = " << tempPopMsg.length << endl;
                    continue;
                }
            } else if (event[i].events & EPOLLOUT) {
                cout << __LINE__ << " fd = " << event[i].data.fd << endl;
                KeyServerEpollMessage_t msgOut;
                epollSessionMutex_.lock();
                msgOut = epollSession_.at(event[i].data.fd);
                epollSessionMutex_.unlock();
                // cout << "KeyServer : fd = " << event[i].data.fd << " start send data, size = " << msgOut.length << endl;
                if (msgOut.fd != event[i].data.fd) {
                    cerr << "KeyServer : epoll event fd not equal to msg-fd " << msgOut.fd << endl;
                    continue;
                }
                if (!keySecurityChannel_->send(sslConnectionList_[event[i].data.fd], (char*)msgOut.keyContent, msgOut.length)) {
                    ev.events = EPOLLHUP;
                    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, event[i].data.fd, &ev) != 0) {
                        cerr << "KeyServer : epoll delete error, fd = " << event[i].data.fd << endl;
                    }
                    epollSessionMutex_.lock();
                    epollSession_.erase(event[i].data.fd);
                    epollSessionMutex_.unlock();
                    cerr << "KeyServer : client closed ssl connect, fd = " << event[i].data.fd << endl;
                    continue;
                }
                ev.data.ptr = (void*)&msgOut;
                ev.data.fd = event[i].data.fd;
                ev.events = EPOLLIN;
                if (epoll_ctl(epfd_, EPOLL_CTL_MOD, event[i].data.fd, &ev) != 0) {
                    cerr << "KeyServer : epoll out change mode error, fd = " << event[i].data.fd << endl;
                }
                cout << "KeyServer : fd = " << event[i].data.fd << " send key done, set EPOLLIN" << endl;
                continue;
            } else {
                continue;
            }
        }
    }
}

void keyServer::runSendThread()
{
    cout << "KeyServer : start epoll collection thread" << endl;
    KeyServerEpollMessage_t msgTemp;
    epoll_event ev;
    while (true) {
        if (responseMQ_->pop(msgTemp)) {
            if (epollSession_.find(msgTemp.fd) == epollSession_.end()) {
                cerr << "find data in epoll session failed" << endl;
                continue;
            } else {
                epollSessionMutex_.lock();
                epollSession_.at(msgTemp.fd).length = msgTemp.length;
                epollSession_.at(msgTemp.fd).keyGenerateFlag = true;
                memcpy(epollSession_.at(msgTemp.fd).keyContent, msgTemp.keyContent, msgTemp.length);
                epollSessionMutex_.unlock();
                ev.data.fd = msgTemp.fd;
                ev.events = EPOLLOUT;
                if (epoll_ctl(epfd_, EPOLL_CTL_MOD, msgTemp.fd, &ev) != 0) {
                    cerr << "KeyServer : epoll out change mode error, fd = " << msgTemp.fd << endl;
                }
                // cout << "fd = " << msgTemp.fd << " generate key done, set EPOLLOUT" << endl;
            }
        }
    }
}

void keyServer::runKeyGenerateRequestThread(int threadID)
{
    cout << "KeyServer : start epoll key generate thread " << threadID << endl;
    while (true) {
        KeyServerEpollMessage_t keyGenTempEpollMessage;
        if (requestMQ_->pop(keyGenTempEpollMessage)) {
            perThreadKeyGenerateCount_[threadID] += keyGenTempEpollMessage.requestNumber;
#if KEY_GEN_SGX_CFB == 1
            client->request(keyGenTempEpollMessage.hashContent, keyGenTempEpollMessage.length, keyGenTempEpollMessage.keyContent, config.getKeyBatchSize() * CHUNK_HASH_SIZE);
#elif KEY_GEN_SGX_CTR == 1
            if (clientList_.find(keyGenTempEpollMessage.clientID) == clientList_.end()) {
                cerr << "find data in client mask info list failed" << endl;
                auto it = clientList_.begin();
                while (it != clientList_.end()) {
                    cout << "KeyServer : client list containes: " << it->first << ", mask info : user counter = " << it->second.keyGenerateCounter << ", current counter = " << it->second.currentKeyGenerateCounter << endl;
                    ++it;
                }
                // requestMQ_->push(keyGenTempEpollMessage);
                continue;
            } else {
                client->request(keyGenTempEpollMessage.hashContent, keyGenTempEpollMessage.length, keyGenTempEpollMessage.keyContent, config.getKeyBatchSize() * CHUNK_HASH_SIZE, clientList_.at(keyGenTempEpollMessage.clientID).clientID, clientList_.at(keyGenTempEpollMessage.clientID).keyGenerateCounter, clientList_.at(keyGenTempEpollMessage.clientID).currentKeyGenerateCounter, clientList_.at(keyGenTempEpollMessage.clientID).nonce, clientList_.at(keyGenTempEpollMessage.clientID).nonceLen);
                clientList_.at(keyGenTempEpollMessage.clientID).keyGenerateCounter += keyGenTempEpollMessage.requestNumber * 4;
                clientList_.at(keyGenTempEpollMessage.clientID).currentKeyGenerateCounter += keyGenTempEpollMessage.requestNumber * 4;
            }
#endif
            responseMQ_->push(keyGenTempEpollMessage);
        }
    }
}

#else

#if KEY_GEN_SGX_CFB == 1

void keyServer::runKeyGenerateThread(SSL* connection)
{
#if SGSTEM_BREAK_DOWN == 1
    struct timeval timestart;
    struct timeval timeend;
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount_++;
    multiThreadCountMutex_.unlock();
    uint64_t currentThreadkeyGenerationNumber = 0;
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            cerr << "keyServer : Thread exit due to client disconnect" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#endif
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#endif
            return;
        }

        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        // cerr << "KeyServer : recv hash number = " << recvNumber << endl;
        u_char key[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
#if KEY_GEN_SGX_MULTITHREAD_ENCLAVE == 1

#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timestart, 0);
#endif
        client->request(hash, recvSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE);
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        multiThreadMutex_.lock();
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();

#elif KEY_GEN_SGX_MULTITHREAD_ENCLAVE == 0

        multiThreadMutex_.lock();
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timestart, 0);
#endif
        client->request(hash, recvSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE);
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();

#endif
        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#endif
            return;
        }
    }
}

#elif KEY_GEN_SGX_CTR == 1

void keyServer::runKeyGenerateThread(SSL* connection)
{
#if SGSTEM_BREAK_DOWN == 1
    struct timeval timestart;
    struct timeval timeend;
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount_++;
    multiThreadCountMutex_.unlock();
    int recvSize = 0;
    uint64_t currentThreadkeyGenerationNumber = 0;
    uint32_t recvedClientCounter = 0;
    MaskInfo_t currentMaskInfo;
    bool clientInfoExistFlag = false;
    // recv init messages
    NetworkHeadStruct_t netHead;
    char initInfoBuffer[16 + sizeof(NetworkHeadStruct_t)]; // clientID & nonce & counter

    if (keySecurityChannel_->recv(connection, initInfoBuffer, recvSize)) {
        memcpy(&netHead, initInfoBuffer, sizeof(NetworkHeadStruct_t));
        memcpy(&recvedClientCounter, initInfoBuffer + sizeof(NetworkHeadStruct_t), sizeof(uint32_t));
        memcpy(currentMaskInfo.nonce, initInfoBuffer + sizeof(NetworkHeadStruct_t) + sizeof(uint32_t), 16 - sizeof(uint32_t));
        currentMaskInfo.clientID = netHead.clientID;
        currentMaskInfo.keyGenerateCounter = 0; // counter for AES block size, each key increase 4 (hash + key) of the counter
        currentMaskInfo.currentKeyGenerateCounter = 0;
        multiThreadCountMutex_.lock();
        if (clientList_.find(currentMaskInfo.clientID) == clientList_.end()) {
            cerr << "KeyServer : find data in client mask info list failed" << endl;
        } else {
            if (clientList_.at(currentMaskInfo.clientID).keyGenerateCounter == recvedClientCounter) {
                cout << "KeyServer : compared key generate encryption counter success" << endl;
                clientInfoExistFlag = true;
            } else {
                cerr << "KeyServer : compared key generate encryption counter error, recved counter = " << recvedClientCounter << ", exist counter = " << clientList_.at(currentMaskInfo.clientID).keyGenerateCounter << endl;
                clientList_.erase(clientID);
                return;
            }
        }
        multiThreadCountMutex_.unlock();

    } else {
        cerr << "KeyServer : error recv client init messages" << endl;
        return;
    }
    //done
    while (true) {
        u_char hash[sizeof(NetworkHeadStruct_t) + config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            cerr << "keyServer : Thread exit due to client disconnect" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            if (clientInfoExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentMaskInfo.currentKeyGenerateCounter;
                currentMaskInfo.currentKeyGenerateCounter = 0;
                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
            } else {
                clientList_.at(currentMaskInfo.clientID).keyGenerateCounter += clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter;
                clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter = 0;
            }
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            if (clientInfoExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentMaskInfo.currentKeyGenerateCounter;
                currentMaskInfo.currentKeyGenerateCounter = 0;
                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
            } else {
                clientList_.at(currentMaskInfo.clientID).keyGenerateCounter += clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter;
                clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter = 0;
            }
            multiThreadCountMutex_.unlock();
#endif
            return;
        }
        memcpy(&netHead, hash, sizeof(NetworkHeadStruct_t));
        if ((netHead.dataSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            if (clientInfoExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentMaskInfo.currentKeyGenerateCounter;
                currentMaskInfo.currentKeyGenerateCounter = 0;
                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
            } else {
                clientList_.at(currentMaskInfo.clientID).keyGenerateCounter += clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter;
                clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter = 0;
            }
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            if (clientInfoExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentMaskInfo.currentKeyGenerateCounter;
                currentMaskInfo.currentKeyGenerateCounter = 0;
                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
            } else {
                clientList_.at(currentMaskInfo.clientID).keyGenerateCounter += clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter;
                clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter = 0;
            }
            multiThreadCountMutex_.unlock();
#endif
            return;
        }

        int recvNumber = netHead.dataSize / CHUNK_HASH_SIZE;
        // cerr << "KeyServer : recv hash number = " << recvNumber << endl;
        u_char key[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
#if KEY_GEN_SGX_MULTITHREAD_ENCLAVE == 1

#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timestart, 0);
#endif
        if (clientInfoExistFlag == true) {
            cout << netHead.clientID << ", " << clientList_.at(currentMaskInfo.clientID).keyGenerateCounter << ", " << clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter << ", " << clientList_.at(currentMaskInfo.clientID).nonceLen << endl;
            client->request(hash + sizeof(NetworkHeadStruct_t), netHead.dataSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE, netHead.clientID, clientList_.at(currentMaskInfo.clientID).keyGenerateCounter, clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter, clientList_.at(currentMaskInfo.clientID).nonce, clientList_.at(currentMaskInfo.clientID).nonceLen);
            clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter += 4 * recvNumber;
        } else {
            cout << netHead.clientID << ", " << currentMaskInfo.keyGenerateCounter << ", " << currentMaskInfo.currentKeyGenerateCounter << ", " << currentMaskInfo.nonceLen << endl;
            client->request(hash + sizeof(NetworkHeadStruct_t), netHead.dataSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE, netHead.clientID, currentMaskInfo.keyGenerateCounter, currentMaskInfo.currentKeyGenerateCounter, currentMaskInfo.nonce, currentMaskInfo.nonceLen);
            currentMaskInfo.currentKeyGenerateCounter += 4 * recvNumber;
        }
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        multiThreadMutex_.lock();
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();
#else
        multiThreadMutex_.lock();
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timestart, 0);
#endif
        if (clientInfoExistFlag == true) {
            cout << netHead.clientID << ", " << clientList_.at(currentMaskInfo.clientID).keyGenerateCounter << ", " << clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter << ", " << clientList_.at(currentMaskInfo.clientID).nonceLen << endl;
            client->request(hash + sizeof(NetworkHeadStruct_t), netHead.dataSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE, netHead.clientID, clientList_.at(currentMaskInfo.clientID).keyGenerateCounter, clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter, clientList_.at(currentMaskInfo.clientID).nonce, clientList_.at(currentMaskInfo.clientID).nonceLen);
            clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter += 4 * recvNumber;
        } else {
            cout << netHead.clientID << ", " << currentMaskInfo.keyGenerateCounter << ", " << currentMaskInfo.currentKeyGenerateCounter << ", " << currentMaskInfo.nonceLen << endl;
            client->request(hash + sizeof(NetworkHeadStruct_t), netHead.dataSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE, netHead.clientID, currentMaskInfo.keyGenerateCounter, currentMaskInfo.currentKeyGenerateCounter, currentMaskInfo.nonce, currentMaskInfo.nonceLen);
            currentMaskInfo.currentKeyGenerateCounter += 4 * recvNumber;
        }
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();
#endif
        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            if (clientInfoExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentMaskInfo.currentKeyGenerateCounter;
                currentMaskInfo.currentKeyGenerateCounter = 0;
                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
            } else {
                clientList_.at(currentMaskInfo.clientID).keyGenerateCounter += clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter;
                clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter = 0;
            }
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            if (clientInfoExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentMaskInfo.currentKeyGenerateCounter;
                currentMaskInfo.currentKeyGenerateCounter = 0;
                clientList_.insert(make_pair(currentMaskInfo.clientID, currentMaskInfo));
            } else {
                clientList_.at(currentMaskInfo.clientID).keyGenerateCounter += clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter;
                clientList_.at(currentMaskInfo.clientID).currentKeyGenerateCounter = 0;
            }
            multiThreadCountMutex_.unlock();
#endif
            return;
        }
    }
}

#elif KEY_GEN_SERVER_MLE_NO_OPRF == 1

void keyServer::runKeyGenerateThread(SSL* connection)
{
#if SGSTEM_BREAK_DOWN == 1
    struct timeval timestart;
    struct timeval timeend;
    double keyGenTime = 0;
    long diff;
    double second;
#endif
    multiThreadCountMutex_.lock();
    clientThreadCount_++;
    multiThreadCountMutex_.unlock();
    uint64_t currentThreadkeyGenerationNumber = 0;
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            cerr << "keyServer : Thread exit due to client disconnect" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#endif
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#endif
            return;
        }
        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        // cerr << "KeyServer : recv hash number = " << recvNumber << endl;

        u_char key[config.getKeyBatchSize() * CHUNK_HASH_SIZE];

#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timestart, 0);
#endif
        for (int i = 0; i < recvNumber; i++) {
            u_char tempKeySeed[CHUNK_HASH_SIZE + 64];
            memset(tempKeySeed, 0, CHUNK_HASH_SIZE + 64);
            memcpy(tempKeySeed, hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
            SHA256(tempKeySeed, CHUNK_HASH_SIZE, key + i * CHUNK_ENCRYPT_KEY_SIZE);
        }
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        keyGenerateCount_ += recvNumber;
        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            multiThreadCountMutex_.unlock();
#endif
            return;
        }
    }
}

#endif

#endif