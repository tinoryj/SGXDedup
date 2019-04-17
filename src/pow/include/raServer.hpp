//
// Created by a on 3/20/19.
//

#ifndef GENERALDEDUPSYSTEM_RASERVER_HPP
#define GENERALDEDUPSYSTEM_RASERVER_HPP


#include <sgx_urts.h>
#include <sgx_uae_service.h>
#include <sgx_ukey_exchange.h>

#include "powSession.hpp"
#include "protocol.hpp"
#include "crypto.h"
#include "iasrequest.h"
#include "json.hpp"
#include "byteorder.h"
#include "base64.h"

#include "configure.hpp"

#define CA_BUNDLE   "/etc/ssl/certs/ca-certificates.crt"
#define IAS_SIGNING_CA_FILE "key/AttestationReportSigningCACert.pem"
#define IAS_CERT_FILE "key/iasclient.crt"
#define IAS_CLIENT_KEY "key/iasclient.pem"

//sp private key
static const unsigned char def_service_private_key[32] = {
        0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
        0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
        0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
        0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
};

class raServer{
private:
    bool derive_kdk(EVP_PKEY *Gb, unsigned char kdk[16], sgx_ec256_public_t g_a);
    bool get_sigrl (sgx_epid_group_id_t gid,char **sig_rl, uint32_t *sig_rl_size);
    bool get_attestation_report(const char *b64quote, sgx_ps_sec_prop_desc_t secprop, ra_msg4_t *msg4);

    IAS_Connection *_ias;
    X509 *_signing_ca;
    X509_STORE *_store;
    sgx_spid_t _spid;
    uint16_t _quote_type;
    EVP_PKEY *_service_private_key;
    uint16_t _iasVersion;

public:
    raServer();
    bool process_msg01(powSession *session,sgx_msg01_t &msg01,sgx_ra_msg2_t &msg2);
    bool process_msg3(powSession *session,sgx_ra_msg3_t *msg3, ra_msg4_t &msg4,uint32_t quote_sz);
};

#endif //GENERALDEDUPSYSTEM_RASERVER_HPP
