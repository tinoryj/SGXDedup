//
// Created by a on 1/20/19.
//

#ifndef GENERALDEDUPSYSTEM_POWSERVER_HPP
#define GENERALDEDUPSYSTEM_POWSERVER_HPP

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include "common.h"
#include "hexutil.h"
#include "fileio.h"
#include "base64.h"
#include "logfile.h"
#include "settings.h"

#include "Sock.hpp"
#include "CryptoPrimitive.hpp"
#include "_messageQueue.hpp"
#include "message.hpp"
#include "configure.hpp"
#include <json.hpp>
#include "iasrequest.h"
#include <map>

#include <sgx_key_exchange.h>
#include <sgx_report.h>

#define MESSAGE_HEADER 1

extern Configure config;

struct session_t{
    unsigned char g_a[64];
    unsigned char g_b[64];
    unsigned char kdk[16];
    unsigned char smk[16];
    unsigned char sk[16];
    unsigned char mk[16];
    unsigned char vk[16];
    //sgx_ra_session_t ra;

    int fd;
    int cid;
    struct sockaddr addr;
    uint32_t GID;

    void close(){
        close(fd);
        memset(g_a,0,sizeof(g_a));
        memset(g_b,0,sizeof(g_b));
        memset(sk,0,sizeof(sk));
        memset(vk,0,sizeof(vk));
        memset(mk,0,sizeof(mk));
        memset(kdk,0,sizeof(kdk));
        memset(smk,0,sizeof(smk));
    }
};

extern map<int,session_t>sessions;

struct msg01_t {
    uint32_t msg0_extended_epid_group_id;
    sgx_ra_msg1_t msg1;
};

class powServer{
private:

    CryptoPrimitive *_crypto;
    Sock _socket;

    sgx_spid_t spid;

    EVP_PKEY* _serverPrikey;

    _messageQueue _inputMQ;
    _messageQueue _outputMQ;

    powServer();

    ~powServer();

    IAS_Connection _ias;

    int derive_kdk(EVP_PKEY *Gb, unsigned char kdk[16], sgx_ec256_public_t g_a);

    int get_sigrl (int version, sgx_epid_group_id_t gid,char **sigrl, uint32_t *sig_rl_size);

    int get_attestation_report(int version, const char *b64quote, sgx_ps_sec_prop_desc_t sec_prop,
                               int &msg4, int strict_trust);

    int process_msg01 (msg01_t* msg, sgx_ra_msg2_t *msg2, session_t *session);

    int process_msg3 (sgx_ra_msg3_t *msg3,uint32_t quote_sz, int &msg4, session_t *session);

public:

    void run();
};

powServer::powServer() {
    _crypto=new CryptoPrimitive(SHA256_TYPE);
    _inputMQ.createQueue(DATASR_TO_POWSERVER_MQ,READ_MESSAGE);
    _outputMQ.createQueue(DATASR_IN_MQ,WRITE_MESSAGE);
}

powServer::~powServer() {

}

#endif //GENERALDEDUPSYSTEM_POWSERVER_HPP
