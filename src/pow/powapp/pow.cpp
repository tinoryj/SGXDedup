//
// Created by a on 12/9/18.
//
#include "pow.hpp"
using namespace std;

POW::POW(string ip,int port){
    _inputMQ.createQueue("encoder to POW",READ_MESSAGE);
    _outputMQ.createQueue("POW to sender",WRITE_MESSAGE);
    raProcess(_masterEnclaveID);
    _Socket.init(CLIENTTCP,ip,port);
}


void POW::run() {
    vector<Chunk>chunkList;
    vector<unsigned int>sendList;
    Chunk tmpChunk;
    char signature[128];
    string batchData,batchHash;
    char* dataHeader=new char[sizeof(int)];

    while(1){
        chunkList.clear();
        sendList.clear();
        batchData.clear();
        batchHash.clear();
        for(int i=0;i<_batchSize;i++){
            _inputMQ.pop(tmpChunk);
            chunkList.push_back(tmpChunk);
            int logicDataSize=tmpChunk.getLogicDataSize();
            memcpy(dataHeader,(char*)&logicDataSize,sizeof(int));
            for(int j=0;j<4;j++)batchData+=dataHeader[j];
            batchData+=tmpChunk.getLogicData();
            batchHash+=tmpChunk.getChunkHash();
        }
        ecall_calHash(context,batchData.c_str(),batchData.length(),signature);
        sendList=request(batchHash,signature);
        for(auto it:sendList){
            _outputMQ.push(chunkList[it]);
        }
    }
}

bool POW::raProcess(sgx_enclave_id_t eid) {
    sgx_status_t status,pseStatus;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t *msg2=NULL;
    sgx_ra_msg3_t *msg3=NULL;
    ra_msg4_t *msg4=NULL;
    uint32_t epid=0,msg3_sz;
    size_t msg4_sz=0;
    sgx_ra_context_t ra_ctx=0xaaaaaaaa;
    string buffer;
    status=enclave_ra_init(key,true,*ctx, pseStatus);
    if(status!=SGX_SUCCESS){
        std::cerr<<"Error occur at process challenge message\n";
        return false;
    }

    status=sgx_get_extended_epid_group_id(&epid);
    if(status!=SGX_SUCCESS){
        std::cerr<<"Error occur at get devices GID\n";
        return false;
    }

    status=sgx_ra_get_msg1(ra_ctx,eid,sgx_ra_get_ga,&msg1);
    if(status!=SGX_SUCCESS){
        std::cerr<<"Error occur at get msg1\n";
        return false;
    }

    buffer.resize(sizeof(msg1));
    memcpy(&buffer[0],(char*)&msg1,sizeof(msg1));
    _Socket.Send(buffer);

    _Socket.Recv(buffer);
    if(buffer=="ffff"){
        std::cerr<<"Error occur at reponse server challenge message\n";
        return false;
    }
    _Socket.Recv(buffer);
    msg2=new sgx_ra_msg2_t();
    memcpy((char*)msg2,&buffer[0],sizeof(sgx_ra_msg2_t));


    status=sgx_ra_proc_msg2(ra_ctx,eid,sgx_ra_proc_msg2_trusted,
                            sgx_ra_get_msg3_trusted,msg2,
                            sizeof(sgx_ra_msg2_t)+msg2->sig_rl_size,
                            &msg3,&msg3_sz);
    if(status!=SGX_SUCCESS){
        std::cerr<<"Error occur at process msg2\n";
        return false;
    }

    buffer.resize(sgx_ra_msg3_t);
    memcpy(&buffer[0],(char*)msg3,sizeof(sgx_ra_msg3_t));
    _Socket.Send(buffer);

    _Socket.Recv(buffer);
    msg4=new ra_msg4_t();
    memcpy((char*)msg4,&buffer[0],sizeof(ra_msg4_t));
    if(msg4->status==1){
        std::cerr<<"RA SUCCESS\n";
        return true;
    }

    return false;

}

vector<unsigned int> POW::request(string hashList, string signature) {
    vector<unsigned int>ans;
    string buffer;
    _Socket.Send(hashList);
    _Socket.Send(signature);
    _Socket.Recv(buffer);

    int len=buffer.length();
    for(int i=0;i<len;i+=sizeof(int)){
        unsigned int ind=0;
        for(int j=i;j<i+sizeof(int);j++){
            ind<<=8;
            unsigned char c=buffer[i];
            ind|=(unsigned int)c;
        }
        ans.push_back(ind);
    }
    return ans;
}