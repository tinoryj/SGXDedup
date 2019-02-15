//
// Created by a on 2/14/19.
//

#ifndef GENERALDEDUPSYSTEM_POWCLIENT_HPP
#define GENERALDEDUPSYSTEM_POWCLIENT_HPP
#include "enclave_u.h"
#include <sgx_urts.h>
#include <sgx_uae_service.h>
#include <sgx_ukey_exchange.h>
#include <string>
#include "protocol.hpp"
#include "crypto.h"
#include "sender.hpp"
#include <iostream>
#include "_messageQueue.hpp"
#include "chunk.hpp"

//server public key
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

class powClient {
private:
    bool enclave_trusted;
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;

    bool request(string &logicDataBatch, uint8_t cmac[128]);

public:
    sgx_launch_token_t _token = {0};
    sgx_enclave_id_t _eid;
    sgx_ra_context_t _ctx;
    int updated;

    powClient();

    bool do_attestation();

    void run();
};

#endif //GENERALDEDUPSYSTEM_POWCLIENT_HPP
