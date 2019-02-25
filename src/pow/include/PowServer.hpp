//
// Created by a on 2/16/19.
//

#ifndef GENERALDEDUPSYSTEM_POWSERVER_HPP
#define GENERALDEDUPSYSTEM_POWSERVER_HPP

#include "_messageQueue.hpp"
#include "configure.hpp"
#include "protocol.hpp"
#include "iasrequest.h"
#include "byteorder.h"
#include "json.hpp"
#include "base64.h"
#include "crypto.h"
#include "Socket.hpp"
#include "CryptoPrimitive.hpp"
#include <iostream>

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

extern Configure config;

using namespace std;

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

class powServer{
private:
    struct sgx_msg01_t {
        uint32_t msg0_extended_epid_group_id;
        sgx_ra_msg1_t msg1;
    };
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    _messageQueue _netMQ;
    map<int,powSession*>sessions;
    CryptoPrimitive _crypto;
    void closeSession(int fd);
    bool process_msg01(int fd,sgx_msg01_t &msg01,sgx_ra_msg2_t &msg2);
    bool process_msg3 (powSession *session,sgx_ra_msg3_t *msg3, ra_msg4_t &msg4,uint32_t quote_sz);
    bool process_signedHash( powSession *session,powSignedHash req);
    bool derive_kdk(EVP_PKEY *Gb, unsigned char kdk[16], sgx_ec256_public_t g_a);
    bool get_sigrl (sgx_epid_group_id_t gid,char **sig_rl, uint32_t *sig_rl_size);
    bool get_attestation_report(const char *b64quote, sgx_ps_sec_prop_desc_t secprop, ra_msg4_t *msg4);

public:


    IAS_Connection *_ias;
    X509 *_signing_ca;
    X509_STORE *_store;
    sgx_spid_t _spid;
    uint16_t _quote_type;
    EVP_PKEY *_service_private_key;
    uint16_t _iasVersion;


    powServer();
    void run();
};

#endif //GENERALDEDUPSYSTEM_POWSERVER_HPP
