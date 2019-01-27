//
// Created by a on 12/9/18.
//
#include "powClient.hpp"
using namespace std;

extern Configure config;

powClient::powClient() {
    int updated;
    sgx_launch_token_t token;
    _inputMQ.createQueue("encoder to POW",READ_MESSAGE);
    _outputMQ.createQueue("POW to sender",WRITE_MESSAGE);
    sgx_status_t status;
    status=sgx_create_enclave("enclave.signed.so",SGX_DEBUG_FLAG, \
							&token,&updated,&_masterEnclaveID,NULL);

    if ( status != SGX_SUCCESS ) {
        fprintf(stderr, "sgx_create_enclave: %s: %08x\n",
                config.getEnclaveName(), status);
        if ( status == SGX_ERROR_ENCLAVE_FILE_ACCESS )
            std::cerr<<stderr, "Did you forget to set LD_LIBRARY_PATH?\n";
        return;
    }
    _raContext=0xdeadbeef;
    
    raProcess(_masterEnclaveID);
    _Socket.init(CLIENTTCP,config.getPOWServerIp(),config.getPOWServerPort());
}


void powClient::run() {
    vector<Chunk>chunkList;
    vector<unsigned int>sendList;
    Chunk tmpChunk;
    char signature[128];
    string batchData;
    vector<string>batchHash;
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
            batchHash.push_back(tmpChunk.getChunkHash());
        }
        ecall_calHash(_masterEnclaveID,_raContext,batchData.c_str(),batchData.length(),signature);
        sendList=request(batchHash,signature);
        for(auto it:sendList){
            _outputMQ.push(chunkList[it]);
        }
    }
}
bool powClient::raProcess (sgx_enclave_id_t eid)
{
    sgx_status_t status, sgxrv, pse_status;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t *msg2 = NULL;
    sgx_ra_msg3_t *msg3 = NULL;
    int msg4;
    uint32_t msg0_extended_epid_group_id = 0;
    uint32_t msg3_sz;
    int rv;
    size_t msg4sz = 0;
    bool enclaveTrusted = false; // Not Trusted
//    int b_pse= OPT_ISSET(flags, OPT_PSE);
    int b_pse= 1;

    string buffer;


    /* Executes an ECALL that runs sgx_ra_init() */
    status=enclave_ra_init(def_service_public_key, b_pse, _raContext, pse_status);

    /* Did the ECALL succeed? */
    if ( status != SGX_SUCCESS ) {
        fprintf(stderr, "enclave_ra_init: %08x\n", status);
        return 1;
    }

    /* If we asked for a PSE session, did that succeed? */
    if (b_pse) {
        if ( pse_status != SGX_SUCCESS ) {
            fprintf(stderr, "pse_session: %08x\n", sgxrv);
            return 1;
        }
    }

    /* Did sgx_ra_init() succeed? */
    if ( sgxrv != SGX_SUCCESS ) {
        fprintf(stderr, "sgx_ra_init: %08x\n", sgxrv);
        return 1;
    }

    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if ( status != SGX_SUCCESS ) {
        enclave_ra_close(eid, &sgxrv, _raContext);
        fprintf(stderr, "sgx_get_extended_epid_group_id: %08x\n", status);
        return 1;
    }

    /* Generate msg1 */

    status= sgx_ra_get_msg1(_raContext, eid, sgx_ra_get_ga, &msg1);
    if ( status != SGX_SUCCESS ) {
        enclave_ra_close(eid, &sgxrv, _raContext);
        fprintf(stderr, "sgx_ra_get_msg1: %08x\n", status);
        return 1;
    }


    /*
     * Send msg0 and msg1 concatenated together (msg0||msg1). We do
     * this for efficiency, to eliminate an additional round-trip
     * between client and server. The assumption here is that most
     * clients have the correct extended_epid_group_id so it's
     * a waste to send msg0 separately when the probability of a
     * rejection is astronomically small.
     *
     * If it /is/ rejected, then the client has only wasted a tiny
     * amount of time generating keys that won't be used.
     */

    //serialize msg01
    buffer.resize(sizeof(sgx_ra_msg1_t)+MESSAGE_HEADER);
    buffer[0]=0x00;
    memcpy(&buffer[MESSAGE_HEADER],(void*)&msg1,sizeof(sgx_ra_msg1_t));
    _Socket.Send(buffer);

    std::cerr<<"Waiting for msg2\n";

    /* Read msg2
     *
     * msg2 is variable length b/c it includes the revocation list at
     * the end. msg2 is malloc'd in readZ_msg do free it when done.
     */

    if(!_Socket.Recv(buffer)){
        enclave_ra_close(eid, &sgxrv, _raContext);
        std::cerr<<"protocol error reading msg2\n";
        exit(1);
    }


    /* Process Msg2, Get Msg3  */
    /* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

    msg3 = NULL;

    status = sgx_ra_proc_msg2(_raContext, eid,
                              sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
                              sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
                              &msg3, &msg3_sz);


    if ( status != SGX_SUCCESS ) {
        enclave_ra_close(eid, &sgxrv, _raContext);
        fprintf(stderr, "sgx_ra_proc_msg2: %08x\n", status);

        return false;
    }


    buffer.resize(sizeof(sgx_ra_msg3_t)+MESSAGE_HEADER);
    buffer[0]=0x02;
    _Socket.Send(buffer);
    if ( msg3 ) {
        free(msg3);
        msg3 = NULL;
    }

    /* Read Msg4 provided by Service Provider, then process */

    _Socket.Recv(buffer);
    char wrongBUffer[4]={0xff,0xfe,0xfd,0xfc};
    if(memcmp(wrongBUffer,buffer.c_str(),buffer.length())){
        std::cerr<<"Attestation failed\n";
        exit(1);
    }

    std::cerr<<"Enclave TRUSTED\n";


    enclave_ra_close(eid, &sgxrv, _raContext);

    return true;
}

vector<unsigned int> powClient::request(vector<string>hashList, string signature) {
    vector<unsigned int>ans;
    string buffer;
    buffer=serialize(hashList);
    _Socket.Send(buffer);
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