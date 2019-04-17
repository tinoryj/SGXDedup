//
// Created by a on 3/19/19.
//

#ifndef GENERALDEDUPSYSTEM_KMSERVER_HPP
#define GENERALDEDUPSYSTEM_KMSERVER_HPP

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "protocol.hpp"
#include "iasrequest.h"
#include "sgx_quote.h"
#include "byteorder.h"
#include "json.hpp"
#include "base64.h"
#include "crypto.h"
#include "Socket.hpp"
#include "CryptoPrimitive.hpp"
#include <iostream>

class kmServer{
private:
    powSession session;
    CryptoPrimitive _crypto;
    void closeSession(int fd);
    bool process_msg01(int fd,sgx_msg01_t &msg01,sgx_ra_msg2_t &msg2);
    bool process_msg3 (powSession *session,sgx_ra_msg3_t *msg3, ra_msg4_t &msg4,uint32_t quote_sz);
    bool process_signedHash( powSession *session,powSignedHash req);
    bool derive_kdk(EVP_PKEY *Gb, unsigned char kdk[16], sgx_ec256_public_t g_a);
    bool get_sigrl (sgx_epid_group_id_t gid,char **sig_rl, uint32_t *sig_rl_size);
    bool get_attestation_report(const char *b64quote, sgx_ps_sec_prop_desc_t secprop, ra_msg4_t *msg4);

public:
    void init();
    bool request(string hash,string key);
};

#endif //GENERALDEDUPSYSTEM_KMSERVER_HPP
