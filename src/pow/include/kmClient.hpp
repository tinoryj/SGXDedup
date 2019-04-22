//
// Created by a on 3/19/19.
//

#ifndef GENERALDEDUPSYSTEM_KMCLIENT_HPP
#define GENERALDEDUPSYSTEM_KMCLIENT_HPP
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

#include "raClient.hpp"

class kmClient {
private:
    bool enclave_trusted;
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;

public:

    sgx_enclave_id_t _eid;
    sgx_ra_context_t _ctx;
    raClient _ra;
    kmClient();
    bool request(string &hash, string &key);
    sgx_launch_token_t _token = {0};
    int updated;
    bool do_attestation();
    void run();
};

#endif //GENERALDEDUPSYSTEM_KMCLIENT_HPP
