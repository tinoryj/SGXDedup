//
// Created by a on 1/13/19.
//

#ifndef GENERALDEDUPSYSTEM_POW_HPP
#define GENERALDEDUPSYSTEM_POW_HPP
#include "_messageQueue.hpp"
#include "Sock.hpp"
#include "chunk.hpp"
#include "sgx_urts.h"
#include "sgx_uae_service.h"
#include "sgx_key_exchange.h"
#include "sgx_ukey_exchange.h"

class POW{
private:
    sgx_enclave_id_t _masterEnclaveID;
    bool raProcess(sgx_enclave_id_t eid);
    vector<sgx_enclave_id_t>_eidList;
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    Sock _Socket;
    int _batchSize,_hashLen;
public:
    POW(string ip,int port);
    vector<unsigned int> request(string hashList,string signature);
    void run();
};

#endif //GENERALDEDUPSYSTEM_POW_HPP
