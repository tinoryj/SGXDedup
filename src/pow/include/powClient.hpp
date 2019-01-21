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

#define MESSAGE_HEADER 1


static const sgx_ec256_public_t def_service_public_key = {
        {
                0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
                0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
                0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
                0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
        },
        {
                0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
                0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
                0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
                0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
        }

};

class powClient{
private:
    sgx_enclave_id_t _masterEnclaveID;
    bool raProcess(sgx_enclave_id_t eid);
    vector<sgx_enclave_id_t>_eidList;
    sgx_ra_context_t _raContext;
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    Sock _Socket;
    int _batchSize,_hashLen;
public:
    powClient();
    ~powClient();
    vector<unsigned int> request(string hashList,string signature);
    void run();
};

#endif //GENERALDEDUPSYSTEM_POW_HPP
