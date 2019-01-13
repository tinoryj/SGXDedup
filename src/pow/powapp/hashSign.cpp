//
// Created by a on 1/13/19.
//
#include "hashSign.hpp"

sgx_status_t SGX_CDECL ecall_calHash(char *batchLogicData,unsigned int batchLogicDataSize,char *signature){
    static CryptoPrimitive crypto(SHA1_TYPE);
    int logicDataSize,it=0;
    string data,dadaHash,hashSignature,tmp;
    while(it<batchLogicDataSize){
        memcpy(&logicData[it],(char*)logicDataSize,sizeof(int));
        data.resize(logicDataSize);
        it+=sizeof(int);
        memcpy(&data[0],&batchLogicData[it],sizeof(char)*logicDataSize);
        crypto.generaHash(data,tmp);
        dadaHash+=tmp;
    }
    //sign dataHash
}