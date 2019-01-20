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
#include <sgx_key_exchange.h>
#include <sgx_report.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include "json.hpp"
#include "common.h"
#include "hexutil.h"
#include "fileio.h"
#include "crypto.h"
#include "byteorder.h"
#include "protocol.h"
#include "base64.h"
#include "iasrequest.h"
#include "logfile.h"
#include "settings.h"

#include "Sock.hpp"
#include "CryptoPrimitive.hpp"
#include "_messageQueue.hpp"
#include "message.hpp"
#include "configure.hpp"
#include <map>

#define MESSAGE_HEADER 1
#define Qu

extern Configure config;

struct session_t{
    ra_session_t ra;
    int fd;
    struct sockaddr addr;
    uint32_t GID;
};

map<int,session_t>sessions;

struct msg01_t {
    uint32_t msg0_extended_epid_group_id;
    sgx_ra_msg1_t msg1;
};

class powServer{
private:

    CryptoPrimitive _crypto;
    Sock _socket;

    sgx_spid_t spid;

    EVP_PKEY* _serverPrikey;

    _messageQueue _inputMQ;
    _messageQueue _outputMQ;

    powServer();

    ~powServer();

    IAS_Connection _ias;

    int derive_kdk(EVP_PKEY *Gb, unsigned char kdk[16], sgx_ec256_public_t g_a);

    int get_sigrl (int version, sgx_epid_group_id_t gid,char **sigrl, uint32_t *msg2);


public:
    int process_msg01 (msg01_t* msg, sgx_ra_msg2_t *msg2, session_t *session);
    int process_msg3 (sgx_ra_msg3_t *msg3, sgx_ra_msg4_t *msg4, session_t *session);

    void run();
};

int powServer::process_msg01(msg01_t *msg, sgx_ra_msg2_t *msg2, session_t *session) {
    memset(msg2,0,sizeof(sgx_ra_msg2_t));
    if(msg->msg0_extended_epid_group_id!=0){
        std::cerr<<"msg0 Extended Epid Group ID is not zero.  Abor\n";
        return 0;
    }

    EVP_PKEY *Gb=_crypto.key_generate();
    unsigned char digest[32], r[32], s[32], gb_ga[128];

    if(Gb==NULL){
        std::cerr<<"Could not create a session key\n";
        return 0;
    }

    /*
	 * Derive the KDK from the key (Ga) in msg1 and our session key.
	 * An application would normally protect the KDK in memory to
	 * prevent trivial inspection.
	 */
    if(!derive_kdk(Gb,session->ra.kdk,msg0->msg1.g_a)){
        std::cerr<<"Could not derive KDK\n";
        return 0;
    }

    /*
 	 * Derive the SMK from the KDK
	 * SMK = AES_CMAC(KDK, 0x01 || "SMK" || 0x00 || 0x80 || 0x00)
	 */
    _crypto.cmac128(session->ra.kdk,(unsigned char *)("\x01SMK\x00\x80\x00"),
                    7,session->ra.smk);

    /*
	 * Build message 2
	 *
	 * A || CMACsmk(A) || SigRL
	 * (148 + 16 + SigRL_length bytes = 164 + SigRL_length bytes)
	 *
	 * where:
	 *
	 * A      = Gb || SPID || TYPE || KDF-ID || SigSP(Gb, Ga)
	 *          (64 + 16 + 2 + 2 + 64 = 148 bytes)
	 * Ga     = Client enclave's session key
	 *          (32 bytes)
	 * Gb     = Service Provider's session key
	 *          (32 bytes)
	 * SPID   = The Service Provider ID, issued by Intel to the vendor
	 *          (16 bytes)
	 * TYPE   = Quote type (0= linkable, 1= linkable)
	 *          (2 bytes)
	 * KDF-ID = (0x0001= CMAC entropy extraction and key derivation)
	 *          (2 bytes)
	 * SigSP  = ECDSA signature of (Gb.x || Gb.y || Ga.x || Ga.y) as r || s
	 *          (signed with the Service Provider's private key)
	 *          (64 bytes)
	 *
	 * CMACsmk= AES-128-CMAC(A)
	 *          (16 bytes)
	 *
	 * || denotes concatenation
	 *
	 * Note that all key components (Ga.x, etc.) are in little endian
	 * format, meaning the byte streams need to be reversed.
	 *
	 * For SigRL, send:
	 *
	 *  SigRL_size || SigRL_contents
	 *
	 * where sigRL_size is a 32-bit uint (4 bytes). This matches the
	 * structure definition in sgx_ra_msg2_t
	 */

    _crypto.key_to_sgx_ec256(&msg2->g_b,Gb);
    memcpy(&msg2->spid,&this->spid,sizeof(sgx_spid_t));
    msg2->quote_type=config.getQuoteType();
    msg2->kdf_id=1;
    char *sigrl=NULL;
    if(!get_sigrl(config.getIasVersion(),msg->msg1.gid,msg2->sig_rl,&msg2->sig_rl_size)){
        std::cerr<<"could not retrieve the sigrl\n";
        return 0;
    }

    //???
    memcpy(gb_ga,&msg2->g_b,64);
    memcpy(session->ra.g_b,&msg2->g_b,64);
    memcpy(&gb_ga[64],msg->msg1.g_a,64);
    memcpy(session->ra.g_a,&msg->msg1.g_a,64);

    ecdsa_sign(gb_ga,128,_serverPrikey,r,s,digest);
    _crypto.reverse_bytes(&msg2->sign_gb_ga.x,r,32);
    _crypto.reverse_bytes(&msg2->sign_gb_ga.y,s,32);

    _crypto.cmac128(session->ra.smk,(unsigned char*)msg2,148,
                    (unsigned char*)&msg2->mac);
}

int powServer::process_msg3(sgx_ra_msg3_t *msg3, sgx_ra_msg4_t *msg4, session_t *session) {

}

void powServer::run() {
    static msg2FixedSize =  sizeof(sgx_ec256_public_t) +
                            sizeof(sgx_spid_t) +
                            sizeof(uint32_t) +
                            sizeof(sgx_ec256_signature_t) +
                            sizeof(sgx_mac_t) +
                            sizeof(uint32_t);

    epoll_message epoll_msg;
    msg01_t msg01;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t msg2;
    sgx_ra_msg4_t msg4;
    ra_session_t session;

    while(1){
        _inputMQ.pop(msg);
        switch (msg._data[0]){

            //msg01
            case 0x00:{
                memcpy(&msg01,&msg.data[MESSAGE_HEADER],msg.data.length()-MESSAGE_HEADER);
                process_msg01(&msg1,&msg2,&session);
                session.GID=msg01.msg0_extended_epid_group_id;
                sessions[msg.fd]=session;

                //serialize msg2
                msg.data.resize(msg2FixedSize);
                memcpy(&msg.data[0],&msg2,msg2FixedSize);
                msg.data.append(msg2.sig_rl);

                //send to client
                _outputMQ.push(msg);
                continue;
            }

            //msg3
            case 0x02:{
                memcpy(&msg3,&msg.data[MESSAGE_HEADER],msg.data.length()-MESSAGE_HEADER);
                session=sessions[msg._fd];
                process_msg3(&msg3,&msg4,&session);
                sessions[msg._fd]=session;
            }
        }
    }
}

#endif //GENERALDEDUPSYSTEM_POWSERVER_HPP
