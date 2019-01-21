//
// Created by a on 1/21/19.
//

#include <powServer.hpp>


int powServer::derive_kdk(EVP_PKEY *Gb, unsigned char *kdk, sgx_ec256_public_t g_a) {
    unsigned char *Gab_x;
    size_t slen;
    EVP_PKEY *Ga;
    unsigned char cmackey[16];

    memset(cmackey, 0, 16);

    /*
     * Compute the shared secret using the peer's public key and a generated
     * public/private key.
     */

    Ga= _crypto->key_from_sgx_ec256(&g_a);
    if ( Ga == nullptr ) {
        std::cerr<<"key_from_sgx_ec256\n";
        return 0;
    }

    /* The shared secret in a DH exchange is the x-coordinate of Gab */
    Gab_x= _crypto->key_shared_secret(Gb, Ga, &slen);
    if ( Gab_x == nullptr ) {
        std::cerr<<"key_shared_secret\n";
        return 0;
    }

    /* We need it in little endian order, so reverse the bytes. */
    /* We'll do this in-place. */

    _crypto->reverse_bytes(Gab_x, Gab_x, slen);

    /* Now hash that to get our KDK (Key Definition Key) */

    /*
     * KDK = AES_CMAC(0x00000000000000000000000000000000, secret)
     */

    _crypto->cmac128(cmackey, Gab_x, slen, kdk);

    return 1;
}

int powServer::get_sigrl(int version, uint8_t *gid, char **sigrl, uint32_t *sig_rl_size) {
    IAS_Request *req= nullptr;
    int oops= 1;
    string sigrlstr;

    try {
        oops= 0;
        req= new IAS_Request(&_ias, (uint16_t) version);
    }
    catch (...) {
        oops = 1;
    }

    if (oops) {
        std::cerr<<"Exception while creating IAS request object\n";
        return 0;
    }

    if ( req->sigrl(*(uint32_t *) gid, sigrlstr) != IAS_OK ) {
        return 0;
    }

    *sigrl= strdup(sigrlstr.c_str());
    if ( *sigrl == nullptr ) return 0;

    *sig_rl_size= (uint32_t ) sigrlstr.length();

    return 1;
}

int powServer::get_attestation_report(int version, const char *b64quote, sgx_ps_sec_prop_desc_t sec_prop, int &msg4,
                                      int strict_trust) {
    IAS_Request *req = nullptr;
    map<string,string> payload;
    vector<string> messages;
    ias_error_t status;
    string content;

    try {
        req= new IAS_Request(&_ias, (uint16_t) version);
    }
    catch (...) {
        std::cerr<<"Exception while creating IAS request object\n";
        return 0;
    }

    payload.insert(make_pair("isvEnclaveQuote", b64quote));

    status= req->report(payload, content, messages);
    if ( status == IAS_OK ) {

        json::JSON reportObj = json::JSON::Load(content);

        /*
         * If the report returned a version number (API v3 and above), make
         * sure it matches the API version we used to fetch the report.
         *
         * For API v3 and up, this field MUST be in the report.
         */

        if ( reportObj.hasKey("version") && (version!=(unsigned int) reportObj["version"].ToInt()) ) {
            return 0;
        } else if ( version >= 3 ) {
            eprintf("attestation report version required for API version >= 3\n");
            return 0;
        }

        /*
         * This sample's attestion policy is based on isvEnclaveQuoteStatus:
         *
         *   1) if "OK" then return "Trusted"
         *
          *   2) if "CONFIGURATION_NEEDED" then return
         *       "NotTrusted_ItsComplicated" when in --strict-trust-mode
         *        and "Trusted_ItsComplicated" otherwise
         *
         *   3) return "NotTrusted" for all other responses
         *
         *
         * ItsComplicated means the client is not trusted, but can
         * conceivable take action that will allow it to be trusted
         * (such as a BIOS update).
          */

        /*
         * Simply check to see if status is OK, else enclave considered
         * not trusted
         */


        if ( !(reportObj["isvEnclaveQuoteStatus"].ToString().compare("OK"))) {
            msg4=1;
        } else if ( !(reportObj["isvEnclaveQuoteStatus"].ToString().compare("CONFIGURATION_NEEDED"))) {
            msg4=0;
            /*if ( strict_trust ) {
                msg4=0;
                // msg4->status = NotTrusted_ItsComplicated;
            } else {
                msg4=0;
                //msg4->status = Trusted_ItsComplicated;
            }
             */
        } else if ( !(reportObj["isvEnclaveQuoteStatus"].ToString().compare("GROUP_OUT_OF_DATE"))) {
            msg4=0;
            // msg4->status = NotTrusted_ItsComplicated;
        } else {
            msg4=0;
            //msg4->status = NotTrusted;
        }


        return 1;
    }


    switch(status) {
        case IAS_QUERY_FAILED:
            std::cerr<<"Could not query IAS\n";
            break;
        case IAS_BADREQUEST:
            std::cerr<<"Invalid payload\n";
            break;
        case IAS_UNAUTHORIZED:
            std::cerr<<"Failed to authenticate or authorize request\n";
            break;
        case IAS_SERVER_ERR:
            std::cerr<<"An internal error occurred on the IAS server\n";
            break;
        case IAS_UNAVAILABLE:
            std::cerr<<"Service is currently not able to process the request. Try again later.\n";
            break;
        case IAS_INTERNAL_ERROR:
            std::cerr<<"An internal error occurred while processing the IAS response\n";
            break;
        case IAS_BAD_CERTIFICATE:
            std::cerr<<"The signing certificate could not be validated\n";
            break;
        case IAS_BAD_SIGNATURE:
            std::cerr<<"The report signature could not be validated\n";
            break;
        default:
            if ( status >= 100 && status < 600 ) {
                std::cerr<<"Unexpected HTTP response code\n";
            } else {
                std::cerr<<"An unknown error occurred.\n";
            }
    }

    return 0;
}

int powServer::process_msg01(msg01_t *msg, sgx_ra_msg2_t *msg2, session_t *session) {
    memset(msg2,0,sizeof(sgx_ra_msg2_t));
    if(msg->msg0_extended_epid_group_id!=0){
        std::cerr<<"msg0 Extended Epid Group ID is not zero.  Abor\n";
        return 0;
    }

    EVP_PKEY *Gb=_crypto->key_generate();
    unsigned char digest[32], r[32], s[32], gb_ga[128];

    if(Gb==nullptr){
        std::cerr<<"Could not create a session key\n";
        return 0;
    }

    /*
	 * Derive the KDK from the key (Ga) in msg1 and our session key.
	 * An application would normally protect the KDK in memory to
	 * prevent trivial inspection.
	 */
    if(!derive_kdk(Gb,session->kdk,msg->msg1.g_a)){
        std::cerr<<"Could not derive KDK\n";
        return 0;
    }

    /*
 	 * Derive the SMK from the KDK
	 * SMK = AES_CMAC(KDK, 0x01 || "SMK" || 0x00 || 0x80 || 0x00)
	 */
    _crypto->cmac128(session->kdk,(unsigned char *)("\x01SMK\x00\x80\x00"),
                    7,session->smk);

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

    _crypto->key_to_sgx_ec256(&msg2->g_b,Gb);
    memcpy(&msg2->spid,&this->spid,sizeof(sgx_spid_t));
    msg2->quote_type=config.getQuoteType();
    msg2->kdf_id=1;
    if(!get_sigrl(config.getIasVersion(),msg->msg1.gid,(char**)&msg2->sig_rl,&msg2->sig_rl_size)){
        std::cerr<<"could not retrieve the sigrl\n";
        return 0;
    }

    //???
    memcpy(gb_ga,&msg2->g_b,64);
    memcpy(session->g_b,&msg2->g_b,64);
    memcpy(&gb_ga[64],(void*)&msg->msg1.g_a,64);
    memcpy(session->g_a,&msg->msg1.g_a,64);

    _crypto->ecdsa_sign(gb_ga,128,_serverPrikey,r,s,digest);
    _crypto->reverse_bytes(&msg2->sign_gb_ga.x,r,32);
    _crypto->reverse_bytes(&msg2->sign_gb_ga.y,s,32);

    _crypto->cmac128(session->smk,(unsigned char*)msg2,148,
                    (unsigned char*)&msg2->mac);
}

int powServer::process_msg3(sgx_ra_msg3_t *msg3,uint32_t quote_sz, int &msg4, session_t *session) {
    sgx_quote_t *q;
    sgx_mac_t vrfymac;
    char *b64quote;

    if(CRYPTO_memcmp(&msg3->g_a,session->g_a,sizeof(sgx_ec256_public_t))){
        std::cerr<<"msg1.g_a and mgs3.g_a keys don't match\n";
        return 0;
    }

    _crypto->cmac128(session->smk,(unsigned char*)&msg3->g_a,
                    sizeof(sgx_ra_msg3_t)-sizeof(sgx_mac_t)+quote_sz,
                    (unsigned char*)vrfymac);

    if(CRYPTO_memcmp(msg3->mac,vrfymac,sizeof(sgx_mac_t))){
        std::cerr<<"Failed to verify msg3 MAC\n";
        return 0;
    }

    b64quote=_crypto->base64_encode((char*)&msg3->quote,quote_sz);
    q=(sgx_quote_t*)msg3->quote;

    if(memcpy(&session->GID,&q->epid_group_id,sizeof(sgx_epid_group_id_t))){
        std::cerr<<"EPID GID mismatch. Attestation failed.\n";
        return 0;
    }

    if(get_attestation_report(config.getIasVersion(),b64quote,
                              msg3->ps_sec_prop,msg4,1)) {
        unsigned char vfy_rdata[64];
        unsigned char msg_rdata[144]; /* for Ga || Gb || VK */

        sgx_report_body_t *r= &q->report_body;

        memset(vfy_rdata, 0, 64);

        /*
         * Verify that the first 64 bytes of the report data (inside
         * the quote) are SHA256(Ga||Gb||VK) || 0x00[32]
         *
         * VK = CMACkdk( 0x01 || "VK" || 0x00 || 0x80 || 0x00 )
         *
         * where || denotes concatenation.
         */

        /* Derive VK */

        _crypto->cmac128(session->kdk, (unsigned char *)("\x01VK\x00\x80\x00"),
                6, session->vk);

        /* Build our plaintext */

        memcpy(msg_rdata, session->g_a, 64);
        memcpy(&msg_rdata[64], session->g_b, 64);
        memcpy(&msg_rdata[128], session->vk, 16);

        /* SHA-256 hash */

        _crypto->sha256_digest(msg_rdata, 144, vfy_rdata);

        if ( CRYPTO_memcmp((void *) vfy_rdata, (void *) &r->report_data,
                           64) ) {

            std::cerr<<"Report verification failed.\n";
            return 0;
        }

        //?????
        /*
         * A real service provider would validate that the enclave
         * report is from an enclave that they recognize. Namely,
         * that the MRSIGNER matches our signing key, and the MRENCLAVE
         * hash matches an enclave that we compiled.
         *
         * Other policy decisions might include examining ISV_SVN to
         * prevent outdated/deprecated software from successfully
         * attesting, and ensuring the TCB is not out of date.
         */


        /*
         * If the enclave is trusted, derive the MK and SK. Also get
         * SHA256 hashes of these so we can verify there's a shared
         * secret between us and the client.
         */

        if ( msg4 ) {
            unsigned char hashmk[32], hashsk[32];

            _crypto->cmac128(session->kdk, (unsigned char *)("\x01MK\x00\x80\x00"),
                    6, session->mk);
            _crypto->cmac128(session->kdk, (unsigned char *)("\x01SK\x00\x80\x00"),
                    6, session->sk);

            _crypto->sha256_digest(session->mk, 16, hashmk);
            _crypto->sha256_digest(session->sk, 16, hashsk);

        }

    }else{
        std::cerr<<"Attestation failed\n";
        return 0;
    }

    return 1;
}

void powServer::run() {
    static msg2FixedSize =  sizeof(sgx_ec256_public_t) +
                            sizeof(sgx_spid_t) +
                            sizeof(uint32_t) +
                            sizeof(sgx_ec256_signature_t) +
                            sizeof(sgx_mac_t) +
                            sizeof(uint32_t);

    epoll_message msg;
    msg01_t msg01;
    sgx_ra_msg2_t msg2;
    sgx_ra_msg3_t msg3;
    int msg4;
    session_t session;
    int status;

    while(1){
        _inputMQ.pop(msg);
        switch (msg._data[0]){

            //msg01
            case 0x00:{
                memcpy(&msg01,&msg._data[MESSAGE_HEADER],msg._data.length()-MESSAGE_HEADER);
                status=process_msg01(&msg01,&msg2,&session);
                if(status==0){
                    char wrongBUffer[4]={0xff,0xfe,0xfd,0xfc};
                    msg._data=wrongBUffer;
                    _outputMQ.push(msg);
                    continue;
                }
                session.GID=msg01.msg0_extended_epid_group_id;
                sessions[msg._fd]=session;

                //serialize msg2
                msg._data.resize(msg2FixedSize);
                memcpy(&msg._data[0],&msg2,msg2FixedSize);
                msg._data.append((char*)msg2.sig_rl);

                //send to client
                _outputMQ.push(msg);
                continue;
            }

                //msg3
            case 0x02:{
                memcpy(&msg3,&msg._data[MESSAGE_HEADER],msg._data.length()-MESSAGE_HEADER);
                session=sessions[msg._fd];
                status=process_msg3(&msg3,msg._data.length()-MESSAGE_HEADER,msg4,&session);


                if(status==0){
                    char wrongBUffer[4]={0xff,0xfe,0xfd,0xfc};
                    msg._data=wrongBUffer;
                    _outputMQ.push(msg);
                    sessions[msg._fd].close();
                    sessions.erase(msg._fd);
                    continue;
                }
                sessions[msg._fd]=session;

                //send msg4 to client
                msg._data.resize(MESSAGE_HEADER+sizeof(msg4));
                msg._data[0]=0x03;
                memcpy(&msg._data[MESSAGE_HEADER],(void*)&msg4,sizeof(msg4));
                _outputMQ.push(msg4);
                continue;
            }
        }
    }
}