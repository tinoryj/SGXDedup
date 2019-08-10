#include "dedupCore.hpp"

extern Database fp2ChunkDB;
extern Configure config;

void PRINT_BYTE_ARRAY_DEDUP_CORE(
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

DedupCore::DedupCore(DataSR* dataSRTemp, Timer* timerObjTemp)
{
    dataSRObj_ = dataSRTemp;
    cryptoObj_ = new CryptoPrimitive();
    timerObj_ = timerObjTemp;
    timerObj_->startTimer();
}

DedupCore::~DedupCore()
{
    if (cryptoObj_ != nullptr)
        delete cryptoObj_;
}

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
                // cerr << "SGX_SIGNED_HASH_TO_DEDUPCORE data size = " << epollMessageTemp.dataSize << endl;
                // PRINT_BYTE_ARRAY_DEDUP_CORE(stderr, epollMessageTemp.data, epollMessageTemp.dataSize);
                for (int i = 0; i < hashNumber; i++) {
                    string hashTemp((char*)epollMessageTemp.data + sizeof(uint8_t) * 16 + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
                    powSignedHashTemp.hash_.push_back(hashTemp);
                    hashTemp.clear();
                }

                if (this->dedupStage1(powSignedHashTemp, requiredChunkTemp)) {
                    epollMessageTemp.type = SUCCESS;
                    epollMessageTemp.dataSize = sizeof(int) + requiredChunkTemp.size() * sizeof(uint32_t);
                    int requiredChunkNumber = requiredChunkTemp.size();
                    memcpy(epollMessageTemp.data, &requiredChunkNumber, sizeof(int));
                    for (int i = 0; i < requiredChunkNumber; i++) {
                        memcpy(epollMessageTemp.data + sizeof(int) + i * sizeof(uint32_t), &requiredChunkTemp[i], sizeof(uint32_t));
                    }
                } else {
                    cerr << "DedupCore : recv sgx signed hash success, dedup stage report error" << endl;
                    epollMessageTemp.type = ERROR_RESEND;
                    epollMessageTemp.dataSize = 0;
                }
                insertMQToDataSR_CallBack(epollMessageTemp);
                break;
            }
            case CLIENT_UPLOAD_CHUNK: {
                if (this->dedupStage2(epollMessageTemp)) {
                    epollMessageTemp.type = SUCCESS;
                } else {
                    cerr << "DedupCore : dedup stage 2 report error" << endl;
                    epollMessageTemp.type = ERROR_RESEND;
                }
                epollMessageTemp.dataSize = 0;
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

    string tmpdata;
    int size = in.hash_.size();
    for (int i = 0; i < size; i++) {
        if (fp2ChunkDB.query(in.hash_[i], tmpdata)) {
            continue;
        }
        out.push_back(i);
        sig.hashList.push_back(in.hash_[i]);
    }

    sig.startTime = std::chrono::high_resolution_clock::now();
    sig.outDataTime = (int)((double)sig.hashList.size() * config.getTimeOutScale());
    cerr << "DedupCore : regist " << setbase(10) << sig.hashList.size() << " chunk to Timer" << endl
         << "outdate time = " << setbase(10) << sig.outDataTime << endl;
    timerObj_->registerHashList(sig);

    return status;
}

bool DedupCore::dedupStage2(EpollMessage_t& epollMessageTemp)
{
    int chunkNumber;
    memcpy(&chunkNumber, epollMessageTemp.data, sizeof(int));
    int readSize = sizeof(int);
    u_char hash[CHUNK_HASH_SIZE];
    timerObj_->mapMutex_.lock();
    for (int i = 0; i < chunkNumber; i++) {
        int currentChunkSize;
        string originHash((char*)epollMessageTemp.data + readSize, CHUNK_HASH_SIZE);
        readSize += CHUNK_HASH_SIZE;
        memcpy(&currentChunkSize, epollMessageTemp.data + readSize, sizeof(int));
        readSize += sizeof(int);
        cryptoObj_->generateHash(epollMessageTemp.data + readSize, currentChunkSize, hash);
        if (memcmp(&originHash[0], hash, CHUNK_HASH_SIZE) != 0) {
            cerr << "DedupCore : client not honest, server recv fake chunk" << endl;
            return false;
        }
        TimerMapNode newNode;
        memcpy(newNode.data, epollMessageTemp.data + readSize, currentChunkSize);
        newNode.done = true;
        newNode.dataSize = currentChunkSize;
        timerObj_->chunkTable.insert(make_pair(originHash, newNode));

        readSize += currentChunkSize;
    }
    timerObj_->mapMutex_.unlock();

    cerr << "DedupCore : recv " << setbase(10) << chunkNumber << " chunk from client" << endl;
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