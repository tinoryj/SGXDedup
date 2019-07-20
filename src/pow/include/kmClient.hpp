//
// Created by a on 3/19/19.
//

#ifndef GENERALDEDUPSYSTEM_KMCLIENT_HPP
#define GENERALDEDUPSYSTEM_KMCLIENT_HPP
#include "Socket.hpp"
#include "crypto.h"
#include "km_enclave_u.h"
#include "messageQueue.hpp"
#include "powSession.hpp"
#include "protocol.hpp"
#include "pthread.h"
#include "sender.hpp"
#include <iostream>
#include <sgx_uae_service.h>
#include <sgx_ukey_exchange.h>
#include <sgx_urts.h>
#include <string>

static const sgx_ec256_public_t def_service_public_key = {
    { 0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38 },
    { 0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06 }

};

class kmClient {
private:
    bool enclave_trusted;
    sgx_enclave_id_t _eid;
    sgx_ra_context_t _ctx;
    string _keyd, _keyn;
    sgx_launch_token_t _token = { 0 };
    int updated;
    Socket _socket;

public:
    kmClient(string keyn, string keyd);
    bool init(Socket socket);
    bool trusted();
    bool request(string& hash, string& key);
    bool doAttestation();
    bool createEnclave(sgx_enclave_id_t& eid, sgx_ra_context_t& ctx, string enclaveName);
    bool getMsg01(sgx_enclave_id_t& eid, sgx_ra_context_t& ctx, string& msg01);
    bool processMsg2(sgx_enclave_id_t& eid, sgx_ra_context_t& ctx, string& Msg2, string& msg3);
    void raclose(sgx_enclave_id_t& eid, sgx_ra_context_t& ctx);
};

#endif //GENERALDEDUPSYSTEM_KMCLIENT_HPP
