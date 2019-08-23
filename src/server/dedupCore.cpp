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

DedupCore::DedupCore()
{
    cryptoObj_ = new CryptoPrimitive();
}

DedupCore::~DedupCore()
{
    if (cryptoObj_ != nullptr)
        delete cryptoObj_;
}

bool DedupCore::dedupByHash(powSignedHash_t in, RequiredChunk_t& out)
{
    out.clear();
    string tmpdata;
    int size = in.hash_.size();
    for (int i = 0; i < size; i++) {
        // cout << "query chunk hash" << endl;
        // PRINT_BYTE_ARRAY_DEDUP_CORE(stdout, &in.hash_[i][0], CHUNK_HASH_SIZE);
        if (fp2ChunkDB.query(in.hash_[i], tmpdata)) {
            continue;
        } else {
            out.push_back(i);
        }
    }
    return true;
}
