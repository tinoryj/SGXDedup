#include "dedupCore.hpp"

extern Database fp2ChunkDB;
extern Configure config;

DedupCore::DedupCore(DataSR* dataSRTemp)
{
    dataSRObj_ = dataSRTemp;
    cryptoObj_ = new CryptoPrimitive();
    dataSRObj_->timerObj_->startTimer();
}

DedupCore::~DedupCore()
{
    if (cryptoObj_ != nullptr)
        delete cryptoObj_;
}

/*
 * DedupCore thread handle
 * process two type message:
 *      sgx_signed_hash     : for dedup stage one, regist to timer
 *      client_upload_chunk : for dedup stage two, save chunk to cache
 *
 */
void DedupCore::run()
{
    while (true) {
        EpollMessage_t epollMessageTemp;
        if (extractMQFromDataSR(epollMessageTemp)) {
            switch (epollMessageTemp.type) {
            case SGX_SIGNED_HASH_TO_DEDUPCORE: {
                powSignedHash_t powSignedHashTemp;
                RequiredChunk_t requiredChunkTemp;
                int hashNumber = (epollMessageTemp.dataSize - sizeof(uint8_t) * 16) / CHUNK_HASH_SIZE;
                memcpy(powSignedHashTemp.signature_, epollMessageTemp.data, sizeof(uint8_t) * 16);
                for (int i = 0; i < hashNumber; i++) {
                    string hashTemp((char*)epollMessageTemp.data + sizeof(uint8_t) * 16 + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
                    powSignedHashTemp.hash_.push_back(hashTemp);
                    hashTemp.clear();
                }

                if (this->dedupStage1(powSignedHashTemp, requiredChunkTemp)) {
                    epollMessageTemp.type = SUCCESS;
                    epollMessageTemp.dataSize = sizeof(int) + requiredChunkTemp.size() * sizeof(uint32_t);
                    int requiredChunkNumber = requiredChunkTemp.size();
                    memset(epollMessageTemp.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    memcpy(epollMessageTemp.data, &requiredChunkNumber, sizeof(int));
                    for (int i = 0; i < requiredChunkNumber; i++) {
                        memcpy(epollMessageTemp.data + sizeof(int) + i * sizeof(uint32_t), &requiredChunkTemp[i], sizeof(uint32_t));
                    }
                } else {
                    epollMessageTemp.type = ERROR_RESEND;
                    memset(epollMessageTemp.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    epollMessageTemp.dataSize = 0;
                }
                insertMQToDataSR_CallBack(epollMessageTemp);
                break;
            }
            case CLIENT_UPLOAD_CHUNK: {
                StorageChunkList_t chunkListTemp;
                int chunkNumber;
                memcpy(&chunkNumber, epollMessageTemp.data, sizeof(int));
                int readSize = sizeof(int);
                for (int i = 0; i < chunkNumber; i++) {
                    int currentChunkSize;
                    string chunkHash((char*)epollMessageTemp.data + readSize, CHUNK_HASH_SIZE);
                    readSize += CHUNK_HASH_SIZE;
                    memcpy(&currentChunkSize, epollMessageTemp.data + readSize, sizeof(int));
                    readSize += sizeof(int);
                    string chunkData((char*)epollMessageTemp.data + readSize, currentChunkSize);
                    readSize += currentChunkSize;
                    chunkListTemp.chunkHash.push_back(chunkHash);
                    chunkListTemp.logicData.push_back(chunkData);
                }
                if (this->dedupStage2(chunkListTemp)) {
                    epollMessageTemp.type = SUCCESS;
                } else {
                    epollMessageTemp.type = ERROR_RESEND;
                }
                epollMessageTemp.dataSize = 0;
                memset(epollMessageTemp.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                insertMQToDataSR_CallBack(epollMessageTemp);
                break;
            }
            }
        }
    }
}

bool DedupCore::dedupStage1(powSignedHash_t in, RequiredChunk_t& out)
{
    out.clear();
    bool status = true;
    signedHashList_t sig;

    if (status) {
        string tmpdata;
        int size = in.hash_.size();
        for (int i = 0; i < size; i++) {
            if (fp2ChunkDB.query(in.hash_[i], tmpdata)) {
                continue;
            }
            out.push_back(i);
            sig.hashList.push_back(in.hash_[i]);
        }
    }

    /* register unique chunk list to timer
     * when time out,timer will check all chunk received or nor
     * if yes, then push all chunk to storage
     * or not, then delete(de-refer) all chunk in cache
     */

    sig.startTime = std::chrono::high_resolution_clock::now();
    sig.outDataTime = (int)((double)sig.hashList.size() * config.getTimeOutScale());
    cerr << "DedupCore : regist " << setbase(10) << sig.hashList.size() << " chunk to Timer" << endl;
    dataSRObj_->timerObj_->registerHashList(sig);

    return status;
}

bool DedupCore::dedupStage2(StorageChunkList_t& in)
{
    u_char hash[CHUNK_HASH_SIZE];
    for (int i = 0; i < in.chunkHash.size(); i++) {
        //check chunk hash correct or not
        cryptoObj_->generateHash((u_char*)&in.logicData[i][0], in.logicData[i].length(), hash);
        if (memcmp(&in.chunkHash[i][0], hash, CHUNK_HASH_SIZE) != 0 && i != in.chunkHash.size() - 1) {
            cerr << "DedupCore : client not honest, server recv fake chunk" << endl;
            return false;
        }
    }
    dataSRObj_->timerObj_->cache_->setChunk(in.chunkHash, in.logicData);
    cerr << "DedupCore : recv " << setbase(10) << in.chunkHash.size() << " chunk from client" << endl;
    return true;
}

bool DedupCore::insertMQToDataSR_CallBack(EpollMessage_t& newMessage)
{
    return dataSRObj_->insertMQ2DataSR_CallBack(newMessage);
}

bool DedupCore::extractMQFromDataSR(EpollMessage_t& newMessage)
{
    return dataSRObj_->extractMQ2DedupCore(newMessage);
}