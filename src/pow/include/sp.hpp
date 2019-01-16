//
// Created by a on 1/14/19.
//

#ifndef GENERALDEDUPSYSTEM_SP_HPP
#define GENERALDEDUPSYSTEM_SP_HPP

static const unsigned char def_service_private_key[32] = {
        0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
        0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
        0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
        0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
};

int derive_kdk(EVP_PKEY *Gb, unsigned char kdk[16], sgx_ec256_public_t g_a,
               config_t *config);

int process_msg01 (MsgIO *msg, IAS_Connection *ias, sgx_ra_msg1_t *msg1,
                   sgx_ra_msg2_t *msg2, char **sigrl, config_t *config,
                   ra_session_t *session);

int process_msg3 (MsgIO *msg, IAS_Connection *ias, sgx_ra_msg1_t *msg1,
                  ra_msg4_t *msg4, config_t *config, ra_session_t *session);

int get_sigrl (IAS_Connection *ias, int version, sgx_epid_group_id_t gid,
               char **sigrl, uint32_t *msg2);

int get_attestation_report(IAS_Connection *ias, int version,
                           const char *b64quote, sgx_ps_sec_prop_desc_t sec_prop, ra_msg4_t *msg4,
                           int strict_trust);

int get_proxy(char **server, unsigned int *port, const char *url);


#endif //GENERALDEDUPSYSTEM_SP_HPP
