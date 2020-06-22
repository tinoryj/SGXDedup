#ifndef SGXDEDUP_POWSESSION_HPP
#define SGXDEDUP_POWSESSION_HPP

#include "../../../include/configure.hpp"
#include "../../../include/cryptoPrimitive.hpp"
#include "../../../include/messageQueue.hpp"
#include "../../../include/protocol.hpp"
#include "../../../include/ssl.hpp"
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
    u_char g_a[64];
    u_char g_b[64];
    u_char kdk[16];
    u_char smk[16];
    u_char sk[16];
    u_char mk[16];
    u_char vk[16];
    sgx_ra_msg1_t msg1;
};

#endif //SGXDEDUP_POWSESSION_HPP
