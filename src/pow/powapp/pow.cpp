//
// Created by a on 12/9/18.
//
#include "_messageQueue.hpp"
#include "Sock.hpp"
#include "chunk.hpp"
#include "remoteAttestation.hpp"
#include "sgx_urts.h"
#include "sgx_uae_service.h"

class POW{
private:
    sgx_enclave_id_t _masterEnclaveID;
    vector<sgx_enclave_id_t>_eidList;
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    Sock _con;
public:
    POW(string ip,int port);
    void init();
    vector<unsigned int> request(vector<Chunk>& chunkList,char* signature);
    void run();
};

POW::POW(string ip,int port){
    _inputMQ.createQueue("encoder to POW",READ_MESSAGE);
    _outputMQ.createQueue("POW to sender",WRITE_MESSAGE);
    init();
    _con.init(CLIENTTCP,ip,port);
}

void POW::init() {
    remoteAttestation(_masterEnclaveID);
}

void POW::run() {
    while(1){
        vector<Chunk>chunkList;
        vector<unsigned int>sendList;
        Chunk tmpChunk;
        char signature[128];
        string batchData="";
        for(i=0;i<10;i++){
            _inputMQ.pop(tmpChunk);
            chunkList.push_back(tmpChunk);
            batchData+=tmpChunk.getLogicData();
        }
        ecall_calHash(context,batchData.c_str(),batchData.length(),signature);
        sendList=request(chunkList,signature);
        for(auto it:sendList){
            _outputMQ.push(chunkList[it]);
        }
    }
}

vector<unsigned int> POW::request(vector<Chunk> &chunkList, char *signature) {
    vector<int>ans;
    string buffer;

    //con del hash and signature


    int len=buffer.length();
    for(int i=0;i<len;i+=4){
        unsigned int ind=0;
        for(int j=i;j<i+4;j++){
            ind<<=8;
            unsigned char c=buffer[i];
            ind|=(unsigned int)c;
        }
        ans.push_back(ind);
    }
    return ans;
}