//-s 928A6B0E3CDDAD56EB3BADAA3B63F71F -r -d -v

#include "powClient.hpp"
//TODO:tmp
#include "../include/powClient.hpp"


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
    if (!this->do_attestation()) {
        exit(1);
    }

    while (1) {
        batchChunk.clear();
        batchHash.clear();
        batchChunkLogicData.clear();
        request.hash.clear();

        for (int i = 0, cnt = 0; i < 100000 && cnt < 5; i++) {
            if (_inputMQ.pop(tmpChunk)) {
                cnt = 0;
                request.hash.push_back(tmpChunk.getChunkHash());
                batchChunk.push_back(tmpChunk);
                batchHash.push_back(tmpChunk.getChunkHash());
                int currentBatchLen = batchChunkLogicData.length();
                int newBatchDataLen = tmpChunk.getLogicData().length();
                batchChunkLogicData.resize(currentBatchLen + sizeof(int));
                memcpy(&batchChunkLogicData[currentBatchLen], &newBatchDataLen, sizeof(int));
                batchChunkLogicData += tmpChunk.getLogicData();

                //
                string data, hash;
                CryptoPrimitive crypto;
                data = tmpChunk.getLogicData();
                crypto.sha256_digest(data, hash);
            }
            else{
                cnt++;
            }
        }

        if (batchChunk.empty()) continue;
        if (!this->request(batchChunkLogicData, request.signature)) {
            cerr << "POWClient : sgx request failed\n";
            exit(1);
        }

        sender->sendEnclaveSignedHash(request, lists, netstatus);
        if (netstatus != SUCCESS) {
            cerr << "POWClient : send pow signed hash error\n";
            exit(1);
        }

        cerr << "POWClient : Server need " << lists.size() << " over all " << batchChunk.size() << endl;

        for (auto it:lists) {
            _outputMQ.push(batchChunk[it]);
        }
    }
}

bool powClient::request(string &logicDataBatch, uint8_t cmac[16]) {
    sgx_status_t retval;
    if (!enclave_trusted) {
        cerr << "POWClient : can do a request before pow_enclave trusted\n";
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
        cerr<<"POWClient : ecall failed\n";
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
        cerr << "POWClient : Can not launch pow_enclave : " << enclaveName << endl;
        printf("%08x\n",status);
        return false;
    }

	status = enclave_ra_init(_eid, &sgxrv, def_service_public_key, false,
							 &_ctx, &pse_status);
    if (status != SGX_SUCCESS) {
        cerr << "POWClient : pow_enclave ra init failed : " << status << endl;
        return false;
    }
    if (sgxrv != SGX_SUCCESS) {
        cerr << "POWClient : sgx ra init failed : " << sgxrv << endl;
        return false;
    }

    /* Generate msg0 */

	status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if (status != SGX_SUCCESS) {
        enclave_ra_close(_eid, &sgxrv, _ctx);
        cerr << "POWClient : sgx ge epid failed : " << status << endl;
        return false;
    }

    /* Generate msg1 */

	status = sgx_ra_get_msg1(_ctx, _eid, sgx_ra_get_ga, &msg1);
	if (status != SGX_SUCCESS) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "POWClient : sgx error get msg1\n" << status << endl;
		return false;
	}

	int netstatus;
	if (!sender->sendSGXmsg01(msg0_extended_epid_group_id, msg1, msg2, netstatus)) {
		cerr << "POWClient : send msg01 error : " << netstatus;
		enclave_ra_close(_eid, &sgxrv, _ctx);
		return false;
	}
    else {
        cerr << "POWClient : Send msg01 and Recv msg2 success\n";
    }


    /* Process Msg2, Get Msg3  */
    /* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

	status = sgx_ra_proc_msg2(_ctx, _eid,
							  sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
							  sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
							  &msg3, &msg3_sz);

	if (status != SGX_SUCCESS) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "POWClient : sgx_ra_proc_msg2 : " << status << endl;
		if (msg2 != nullptr) {
			delete msg2;
		}
		return false;
	}
    else {
        cerr << "POWClient : process msg2 success\n";
    }

	if (msg2 != nullptr) {
		delete msg2;
	}

	if (!sender->sendSGXmsg3(*msg3, msg3_sz, msg4, netstatus)) {
		enclave_ra_close(_eid, &sgxrv, _ctx);
		cerr << "POWClient : error send_msg3 : " << netstatus << endl;
		if (msg3 != nullptr) {
			delete msg3;
		}
		return false;
	}
    else {
        cerr << "POWClient : send msg3 and Recv msg4 success\n";
    }

	if (msg3 != nullptr) {
		delete msg3;
	}

	if (msg4->status) {
		cerr << "POWClient : Enclave TRUSTED\n";
	} else if (!msg4->status) {
		cerr << "POWClient : Enclave NOT TRUSTED\n";
		delete msg4;
		enclave_ra_close(_eid, &sgxrv, _ctx);
		return false;
	}

	enclave_trusted = msg4->status;

	delete msg4;
	return true;
}