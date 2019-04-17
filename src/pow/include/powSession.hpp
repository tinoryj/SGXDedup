//
// Created by a on 3/20/19.
//

#ifndef GENERALDEDUPSYSTEM_POWSESSION_HPP
#define GENERALDEDUPSYSTEM_POWSESSION_HPP
#include "_messageQueue.hpp"
#include "configure.hpp"
#include "protocol.hpp"
#include "iasrequest.h"
#include "powSession.hpp"
#include "byteorder.h"
#include "json.hpp"
#include "base64.h"
#include "crypto.h"
#include "Socket.hpp"
#include "CryptoPrimitive.hpp"
#include <iostream>
struct sgx_msg01_t {
    uint32_t msg0_extended_epid_group_id;
    sgx_ra_msg1_t msg1;
};

struct powSession{
    bool enclaveTrusted;
    unsigned char g_a[64];
    unsigned char g_b[64];
    unsigned char kdk[16];
    unsigned char smk[16];
    unsigned char sk[16];
    unsigned char mk[16];
    unsigned char vk[16];
    sgx_ra_msg1_t msg1;
};

#endif //GENERALDEDUPSYSTEM_POWSESSION_HPP
