#ifndef GENERALDEDUPSYSTEM_POWSESSION_HPP
#define GENERALDEDUPSYSTEM_POWSESSION_HPP

#include "../../../include/configure.hpp"
#include "../../../include/cryptoPrimitive.hpp"
#include "../../../include/messageQueue.hpp"
#include "../../../include/protocol.hpp"
#include "../../../include/socket.hpp"
#include "base64.h"
#include "byteorder.h"
#include "crypto.h"
#include "iasrequest.h"
#include "json.hpp"
#include "powSession.hpp"
#include <iostream>

struct sgx_msg01_t {
    uint32_t msg0_extended_epid_group_id;
    sgx_ra_msg1_t msg1;
};

struct powSession {
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
