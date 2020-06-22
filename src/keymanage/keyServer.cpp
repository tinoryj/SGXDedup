#include "keyServer.hpp"
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
    keySecurityChannel_ = keyServerSecurityChannelTemp;
#if KEY_GEN_EPOLL_MODE == 1
    requestMQ_ = new messageQueue<keyServerEpollMesage_t>;
    responseMQ_ = new messageQueue<keyServerEpollMesage_t>;
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

bool keyServer::doRemoteAttestation(ssl* raSecurityChannel, SSL* sslConnection)
{
    if (!client->init(raSecurityChannel, sslConnection)) {
        cerr << "keyServer : enclave not truster" << endl;
        return false;
    } else {
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

#if KEY_GEN_EPOLL_MODE == 1

void runRecvThread()
{
    map<int,SSL*>sslconnection;
    _messageQueue mq(KEYMANGER_SR_TO_KEYGEN,WRITE_MESSAGE);
    epoll_event ev,event[100];
    int epfd,fd;
    message* msg=new message();
    epfd=epoll_create(20);
    msg->fd=_keySecurityChannel->listenFd;
    msg->epfd=epfd;
    ev.data.ptr=(void*)msg;
    ev.events=EPOLLET|EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,_keySecurityChannel->listenFd,&ev);

    while(1){
        int nfd=epoll_wait(epfd,event,20,-1);
        for(int i=0;i<nfd;i++){

            //msg 存在耦合，在数据发送后将 msg 删除
            msg=(message*)event[i].data.ptr;
            if(msg->fd==_keySecurityChannel->listenFd){

                message* msg1=new message();
                std::pair<int,SSL*> con=_keySecurityChannel->sslListen();
                *msg1=*msg;
                msg1->fd=con.first;
                sslconnection[con.first]=con.second;
                ev.data.ptr=(void*)msg1;
                ev.events=EPOLLET|EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,msg1->fd,&ev);
                continue;
            }
            if(event[i].events& EPOLLIN){
                string hash;
                if(!_keySecurityChannel->sslRead(sslconnection[msg->fd],hash)){
                    std::cerr<<"client closed\n";
                    epoll_ctl(msg->epfd,EPOLL_CTL_DEL,msg->fd,&ev);
                    close(msg->fd);
                    delete msg;
                    continue;
                }
                memcpy(msg->hash,hash.c_str(),sizeof(msg->hash));
                mq.push(*msg);
                delete msg;*
                continue;
            }
            if(event[i].events& EPOLLOUT){
                _keySecurityChannel->sslWrite(sslconnection[msg->fd],msg->key);
                ev.events=EPOLLET|EPOLLIN;
                ev.data.ptr=(void*)msg;
                epoll_ctl(epfd,EPOLL_CTL_MOD,msg->fd,&ev);
                continue;
            }
        }
    }
}
void runSendThread()
{
    std::string key;
    epoll_event ev;
    ev.events=EPOLLET|EPOLLOUT;
    while(1){
        message* msg = new message();
        if(!mq.pop(*msg)){
            continue;
        }
        this->workloadProgress(msg->hash,key);
        memcpy(msg->key,key.c_str(),sizeof(msg->key));
        ev.data.ptr=(void*)msg;
        epoll_ctl(msg->epfd,EPOLL_CTL_MOD,msg->fd,&ev);
    }

}
void runKeyGenerateRequestThread()
{
    while (true) {

    }
    keyGenerateCount_ += recvNumber;

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
}

#elif KEY_GEN_MULTI_THREAD_MODE == 1

#if KEY_GEN_SGX_CFB == 1

void keyServer::runKeyGenerateThread(SSL* connection)
{
#if SGSTEM_BREAK_DOWN == 1
    struct timeval timestart;
    struct timeval timeend;
    double keyGenTime = 0;
    long diff;
    double second;
#endif #endif
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

void keyServer::runCTRModeMaskGenerate()
{
    while (true) {
        if (raRequestFlag == false) {
            while (!(clientThreadCount_ == 0))
                ;
            cout << "KeyServer : start compute session encryption mask offline" << endl;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientList[i].keyGenerateCounter > 0) {
                    client->maskGenerate(clientList[i].clientID, clientList[i].keyGenerateCounter, clientList[i].nonce, 16 - sizeof(uint32_t));
                }
            }
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC_);
            xt.sec += 20;
            boost::thread::sleep(xt);
        }
    }
}

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
    uint8_t nonce[16 - sizeof(uint32_t)];
    int nonceLen = 16 - sizeof(uint32_t);
    int clientID = 0;
    uint32_t currentKeyGenerateCount = 0;
    uint32_t recvedClientCounter = 0;
    maskInfo currentMaskInfo;
    // recv init messages
    char initInfoBuffer[16 + sizeof(int)]; // clientID & nonce & counter

    if (keySecurityChannel_->recv(connection, initInfoBuffer, recvSize)) {
        memcpy(&clientID, initInfoBuffer, sizeof(int));
        memcpy(&recvedClientCounter, initInfoBuffer + sizeof(int), sizeof(uint32_t));
        memcpy(nonce, initInfoBuffer + sizeof(int) + sizeof(uint32_t), 16 - sizeof(uint32_t));
        memcpy(currentMaskInfo.nonce, nonce, 12);
        currentMaskInfo.clientID = clientID;
        currentMaskInfo.keyGenerateCounter = 0; // counter for AES block size, each key increase 2 for the counter
        if (recvedClientCounter != 0) {
            multiThreadCountMutex_.lock();
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    if (clientList[i].keyGenerateCounter == recvedClientCounter) {
                        cout << "KeyServer : compared key generate encryption counter success" << endl;
                        break;
                    } else {
                        cerr << "KeyServer : compared key generate encryption counter error, recved counter = " << recvedClientCounter << ", exist counter = " << clientList[i].keyGenerateCounter << endl;
                        return;
                    }
                }
            }
            multiThreadCountMutex_.unlock();
        }
    } else {
        cerr << "KeyServer : error recv client init messages" << endl;
        return;
    }
    //done
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        if (!keySecurityChannel_->recv(connection, (char*)hash, recvSize)) {
            cerr << "keyServer : Thread exit due to client disconnect" << endl;
            multiThreadCountMutex_.lock();

            multiThreadCountMutex_.unlock();
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            bool clientIDExistFlag = false;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    clientList[i].keyGenerateCounter += currentKeyGenerateCount;
                    clientIDExistFlag = true;
                    break;
                }
            }
            if (clientIDExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentKeyGenerateCount;
                clientList.push_back(currentMaskInfo);
            }
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            bool clientIDExistFlag = false;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    clientList[i].keyGenerateCounter += currentKeyGenerateCount;
                    clientIDExistFlag = true;
                    break;
                }
            }
            if (clientIDExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentKeyGenerateCount;
                clientList.push_back(currentMaskInfo);
            }
            multiThreadCountMutex_.unlock();
#endif
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "keyServer : recv chunk hash error : hash size wrong" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            bool clientIDExistFlag = false;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    clientList[i].keyGenerateCounter += currentKeyGenerateCount;
                    clientIDExistFlag = true;
                    break;
                }
            }
            if (clientIDExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentKeyGenerateCount;
                clientList.push_back(currentMaskInfo);
            }
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            bool clientIDExistFlag = false;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    clientList[i].keyGenerateCounter += currentKeyGenerateCount;
                    clientIDExistFlag = true;
                    break;
                }
            }
            if (clientIDExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentKeyGenerateCount;
                clientList.push_back(currentMaskInfo);
            }
            multiThreadCountMutex_.unlock();
#endif
            return;
        }

        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        // cerr << "KeyServer : recv hash number = " << recvNumber << endl;
        u_char key[config.getKeyBatchSize() * CHUNK_HASH_SIZE];

        multiThreadMutex_.lock();
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timestart, 0);
#endif
        client->request(hash, recvSize, key, config.getKeyBatchSize() * CHUNK_HASH_SIZE, clientID, currentMaskInfo.keyGenerateCounter, currentKeyGenerateCount, nonce, nonceLen);
#if SGSTEM_BREAK_DOWN == 1
        gettimeofday(&timeend, 0);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        keyGenTime += second;
#endif
        currentKeyGenerateCount += 4 * recvNumber;
        keyGenerateCount_ += recvNumber;
        multiThreadMutex_.unlock();

        currentThreadkeyGenerationNumber += recvNumber;
        if (!keySecurityChannel_->send(connection, (char*)key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
#if SGSTEM_BREAK_DOWN == 1
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            bool clientIDExistFlag = false;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    clientList[i].keyGenerateCounter += currentKeyGenerateCount;
                    clientIDExistFlag = true;
                    break;
                }
            }
            if (clientIDExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentKeyGenerateCount;
                clientList.push_back(currentMaskInfo);
            }
            cout << "KeyServer : total key generation time = " << keyGenTime << " s" << endl;
            cout << "KeyServer : total key generation number = " << currentThreadkeyGenerationNumber << endl;
            multiThreadCountMutex_.unlock();
#else
            multiThreadCountMutex_.lock();
            clientThreadCount_--;
            bool clientIDExistFlag = false;
            for (int i = 0; i < clientList.size(); i++) {
                if (clientID == clientList[i].clientID) {
                    clientList[i].keyGenerateCounter += currentKeyGenerateCount;
                    clientIDExistFlag = true;
                    break;
                }
            }
            if (clientIDExistFlag == false) {
                currentMaskInfo.keyGenerateCounter += currentKeyGenerateCount;
                clientList.push_back(currentMaskInfo);
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