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

DedupCore::DedupCore(DataSR* dataSRTemp)
{
    dataSRObj_ = dataSRTemp;
    cryptoObj_ = new CryptoPrimitive();
}

DedupCore::~DedupCore()
{
    if (cryptoObj_ != nullptr)
        delete cryptoObj_;
}

void DedupCore::run()
{
    EpollMessage_t epollMessageTemp;
    while (true) {
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
                }

                if (this->dedupByHash(powSignedHashTemp, requiredChunkTemp)) {
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
            }
        }
    }
}

bool DedupCore::dedupByHash(powSignedHash_t in, RequiredChunk_t& out)
{
    out.clear();
    string tmpdata;
    int size = in.hash_.size();
    for (int i = 0; i < size; i++) {
        if (fp2ChunkDB.query(in.hash_[i], tmpdata)) {
            continue;
        }
        out.push_back(i);
    }
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