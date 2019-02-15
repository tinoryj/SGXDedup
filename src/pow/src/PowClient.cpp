

#include "powClient.hpp"

using  namespace std;

extern Sender *sender;
extern Configure config;

void powClient::run() {
    vector<Chunk> batchChunk;
    vector<string> batchHash;
    string batchChunkLogicData;
    powSignedHash request;
    RequiredChunk lists;
    Chunk tmpChunk;
    int netstatus;

    while(1){
        batchChunk.clear();
        batchHash.clear();
        batchChunkLogicData.clear();

        for(int i=0;i<10;i++){
            if(_inputMQ.pop(tmpChunk)){
                batchChunk.push_back(tmpChunk);
                batchHash.push_back(tmpChunk.getChunkHash());
                batchChunkLogicData+=tmpChunk.getLogicData();
            }
        }

        if(batchChunk.empty()) continue;

        if(!this->request(batchChunkLogicData,request.signature)){
            cerr<<"sgx request failed\n";
            exit(1);
        }

        sender->sendEnclaveSignedHash(request,lists,netstatus);

        if(netstatus!=SUCCESS){
            cerr<<"send pow signed hash error\n";
            exit(1);
        }
    }
}

bool powClient::request(string &logicDataBatch, uint8_t cmac[128]) {
	sgx_status_t retval;
	if (!enclave_trusted) {
		cerr << "can do a request before enclave trusted\n";
		return false;
	}

	uint32_t srcLen = logicDataBatch.length();
	sgx_status_t status;
	uint8_t *src = new uint8_t[logicDataBatch.length()];

	if (src == nullptr) {
		cerr << "mem error\n";
		return false;
	}
	memcpy(src, &logicDataBatch[0], logicDataBatch.length());
	status = ecall_calcmac(_eid, &retval,&_ctx, SGX_RA_KEY_SK, src, srcLen, cmac);
	if (status != SGX_SUCCESS) {
		cerr<<"ecall failed\n";
		return false;
	}
	return true;
}

powClient::powClient() {
	updated = 0;
	enclave_trusted = false;
	_ctx = 0xdeadbeef;
    _inputMQ.createQueue(ENCODER_TO_POW_MQ,READ_MESSAGE);
    _outputMQ.createQueue(SENDER_IN_MQ,WRITE_MESSAGE);
}

bool powClient::do_attestation () {
	sgx_status_t status, sgxrv, pse_status;
	sgx_ra_msg1_t msg1;
	sgx_ra_msg2_t *msg2;
	sgx_ra_msg3_t *msg3;
	ra_msg4_t *msg4 = NULL;
	uint32_t msg0_extended_epid_group_id = 0;
	uint32_t msg3_sz;
	int rv;
	size_t msg4sz = 0;

	string enclaveName = config.getEnclaveName();
	status = sgx_create_enclave(enclaveName.c_str(), SGX_DEBUG_FLAG, &_token, &updated, &_eid, 0);
	if (status != SGX_SUCCESS) {
		cerr << "Can not launch enclave : " << enclaveName << endl;
		return false;
	}

	status = enclave_ra_init(_eid, &sgxrv, def_service_public_key, false,
							 &_ctx, &pse_status);
	if (status != SGX_SUCCESS) {
		cerr << "enclave ra init failed : " << status << endl;
		return false;
	}
	if (sgxrv != SGX_SUCCESS) {
		cerr << "sgx ra init failed : " << sgxrv << endl;
		return false;
	}

	/* Generate msg0 */

	status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
	if (status != SGX_SUCCESS) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "sgx ge epid failed : " << status << endl;
		return false;
	}

	/* Generate msg1 */

	status = sgx_ra_get_msg1(_ctx, _eid, sgx_ra_get_ga, &msg1);
	if (status != SGX_SUCCESS) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "sgx error get msg1\n" << status << endl;
		return false;
	}

	int netstatus;
	if (!sender->sendSGXmsg01(msg0_extended_epid_group_id, msg1, msg2, netstatus)) {
		cerr << "send msg01 error : " << netstatus;
		enclave_ra_close(_eid, &sgxrv, _ctx);
		return false;
	}


	/* Process Msg2, Get Msg3  */
	/* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

	status = sgx_ra_proc_msg2(_ctx, _eid,
							  sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
							  sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
							  &msg3, &msg3_sz);

	if (status != SGX_SUCCESS) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "sgx_ra_proc_msg2 : " << status << endl;
		if (msg2 != nullptr) {
			delete msg2;
		}
		return false;
	}

	if (msg2 != nullptr) {
		delete msg2;
	}

	if (!sender->sendSGXmsg3(*msg3, msg3_sz, msg4, netstatus)) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "error send_msg3 : " << netstatus << endl;
		if (msg3 != nullptr) {
			delete msg3;
		}
		return false;
	}

	if (msg3 != nullptr) {
		delete msg3;
	}

	if (msg4->status) {
		cerr << "Enclave TRUSTED\n";
	} else if (!msg4->status) {
		cerr << "Enclave NOT TRUSTED\n";
		delete msg4;
		enclave_ra_close(_eid, &sgxrv, _ctx);
		return false;
	}

	enclave_trusted = msg4->status;

	delete msg4;
	return true;
}