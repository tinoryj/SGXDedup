#include "powServer.hpp"
//./sp -s 928A6B0E3CDDAD56EB3BADAA3B63F71F -A ./client.crt
// -C ./client.crt --ias-cert-key=./client.pem -x -d -v
// -A AttestationReportSigningCACert.pem -C client.crt
// -s 797F0D90EE75B24B554A73AB01FD3335 -Y client.pem
void powServer::closeSession(int fd)
{
    sessions.erase(fd);
}

powServer::powServer(dataSR* dataSRObjTemp)
{

    dataSRObj_ = dataSRObjTemp;

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    if (!cert_load_file(&_signing_ca, IAS_SIGNING_CA_FILE)) {
        cerr << "can not load IAS Signing Cert CA\n";
        exit(1);
    }

    _store = cert_init_ca(_signing_ca);
    if (_store == nullptr) {
        cerr << "can not init cert file\n";
        exit(1);
    }

    string spid = config.getPOWSPID();
    if (spid.length() != 32) {
        cerr << "SPID must be 32-byte hex string\n";
        exit(1);
    }

    from_hexstring((unsigned char*)&_spid, (const void*)&spid[0], 16);

    _ias = new IAS_Connection(config.getPOWIASServerType(), 0);
    _ias->client_cert(IAS_CERT_FILE, "PEM");
    _ias->client_key(IAS_CLIENT_KEY, nullptr);
    _ias->proxy_mode(IAS_PROXY_NONE);
    _ias->cert_store(_store);
    _ias->ca_bundle(CA_BUNDLE);

    _quote_type = config.getPOWQuoteType();
    _service_private_key = key_private_from_bytes(def_service_private_key);
    _iasVersion = config.getPOWIASVersion();
}

void powServer::run()
{
    EpollMessage_t msg;
    sgx_msg01_t msg01;
    sgx_ra_msg2_t msg2;
    sgx_ra_msg3_t* msg3;
    ra_msg4_t msg4;

    while (1) {
        if (!extractMQFromDataSR(msg)) {
            continue;
        }
        switch (msg.type) {
        case SGX_RA_MSG01: {
            memcpy(&msg01, msg.data, sizeof(sgx_msg01_t));
            memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
            if (!process_msg01(msg.fd, msg01, msg2)) {
                cerr << "error process msg01\n";
                msg.type = ERROR_RESEND;
                msg.dataSize = 0;
            } else {
                msg.type = SUCCESS;
                msg.dataSize = sizeof(sgx_ra_msg2_t);
                memcpy(msg.data, &msg2, sizeof(sgx_ra_msg2_t));
            }
            insertMQToDataSR(msg);
            break;
        }
        case SGX_RA_MSG3: {
            msg3 = (sgx_ra_msg3_t*)new char[msg.dataSize];
            memcpy(&msg3, msg.data, msg.dataSize);
            if (sessions.find(msg.fd) == sessions.end()) {
                cerr << "client had not send msg01 before\n";
                msg.type = ERROR_CLOSE;
                msg.dataSize = 0;
                memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
            } else {
                if (this->process_msg3(sessions[msg.fd], msg3, msg4,
                        msg.dataSize - sizeof(sgx_ra_msg3_t))) {
                    msg.type = SUCCESS;
                    msg.dataSize = sizeof(ra_msg4_t);
                    memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    memcpy(msg.data, &msg4, sizeof(ra_msg4_t));
                } else {
                    cerr << "sgx msg3 error\n";
                    msg.type = ERROR_CLOSE;
                    msg.dataSize = 0;
                    memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                }
            }
            insertMQToDataSR(msg);
            break;
        }
        case SGX_SIGNED_HASH: {
            powSignedHash clientReq;
            int signedHashNumber = (msg.dataSize - sizeof(uint8_t) * 16) / CHUNK_HASH_SIZE;
            memcpy(clientReq.signature_, msg.data, sizeof(uint8_t) * 16);
            memset(msg.data, 0, EPOLL_MESSAGE_DATA_SIZE);
            for (int i = 0; i < signedHashNumber; i++) {
                string hash;
                hash.resize(CHUNK_HASH_SIZE);
                memcpy(&hash[0], msg.data + sizeof(uint8_t) * 16 + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
                clientReq.hash_.push_back(hash);
            }

            if (sessions.find(msg.fd) == sessions.end()) {
                cerr << "client not trusted yet\n";
                msg.type = ERROR_CLOSE;
                msg.dataSize = 0;
            } else {
                if (!sessions.at(msg.fd)->enclaveTrusted) {
                    cerr << "client not trusted yet\n";
                    msg.type = ERROR_CLOSE;
                    msg.dataSize = 0;
                } else {
                    if (this->process_signedHash(sessions.at(msg.fd), clientReq)) {
                        msg.type = SGX_SIGNED_HASH_TO_DEDUPCORE;
                        dataSRObj_->insertMQ2DedupCore(msg);
                        break;
                    } else {
                        msg.type = ERROR_RESEND;
                    }
                }
            }
            insertMQToDataSR(msg);
            break;
        }
        case POW_CLOSE_SESSION: {
            this->closeSession(msg.fd);
            break;
        }
        default: {
            break;
        }
        }
    }
}

bool powServer::process_msg01(int fd, sgx_msg01_t& msg01, sgx_ra_msg2_t& msg2)
{
    powSession* current = new powSession();
    sessions.insert(make_pair(fd, current));

    EVP_PKEY* Gb;
    unsigned char digest[32], r[32], s[32], gb_ga[128];

    if (msg01.msg0_extended_epid_group_id != 0) {
        cerr << "msg0 Extended Epid Group ID is not zero.  Exiting.\n";
        return false;
    }

    memcpy(&current->msg1, &msg01.msg1, sizeof(sgx_ra_msg1_t));

    Gb = key_generate();
    if (Gb == nullptr) {
        cerr << "can not create session key\n";
        return false;
    }

    if (!derive_kdk(Gb, current->kdk, msg01.msg1.g_a)) {
        cerr << "can not derive KDK\n";
        return false;
    }

    cmac128(current->kdk, (unsigned char*)("\x01SMK\x00\x80\x00"), 7,
        current->smk);

    //build msg2
    memset(&msg2, 0, sizeof(sgx_ra_msg2_t));

    key_to_sgx_ec256(&msg2.g_b, Gb);
    memcpy(&msg2.spid, &_spid, sizeof(sgx_spid_t));
    msg2.quote_type = _quote_type;
    msg2.kdf_id = 1;

    if (!get_sigrl(msg01.msg1.gid, (char**)&msg2.sig_rl, &msg2.sig_rl_size)) {
        cerr << "can not retrieve sigrl form ias server\n";
        return false;
    }

    memcpy(gb_ga, &msg2.g_b, 64);
    memcpy(current->g_b, &msg2.g_b, 64);

    memcpy(&gb_ga[64], &current->msg1.g_a, 64);
    memcpy(current->g_a, &current->msg1.g_a, 64);

    ecdsa_sign(gb_ga, 128, _service_private_key, r, s, digest);
    reverse_bytes(&msg2.sign_gb_ga.x, r, 32);
    reverse_bytes(&msg2.sign_gb_ga.y, s, 32);

    cmac128(current->smk, (unsigned char*)&msg2, 148, (unsigned char*)&msg2.mac);

    return true;
}

bool powServer::derive_kdk(EVP_PKEY* Gb, unsigned char* kdk, sgx_ec256_public_t g_a)
{
    unsigned char* Gab_x;
    unsigned char cmacKey[16];
    size_t len;
    EVP_PKEY* Ga;
    Ga = key_from_sgx_ec256(&g_a);
    if (Ga == nullptr) {
        cerr << "can not get ga from msg1\n";
        return false;
    }
    Gab_x = key_shared_secret(Gb, Ga, &len);
    if (Gab_x == nullptr) {
        cerr << "can not get shared secret\n";
        return false;
    }
    reverse_bytes(Gab_x, Gab_x, len);

    memset(cmacKey, 0, sizeof(cmacKey));
    cmac128(cmacKey, Gab_x, len, kdk);
    return true;
}

bool powServer::get_sigrl(uint8_t* gid, char** sig_rl, uint32_t* sig_rl_size)
{
    IAS_Request* req = nullptr;

    req = new IAS_Request(_ias, _iasVersion);
    if (req == nullptr) {
        cerr << "can not make ias request\n";
        return false;
    }
    string sigrlstr;
    if (req->sigrl(*(uint32_t*)gid, sigrlstr) != IAS_OK) {
        cerr << "ias get sigrl error\n";
        return false;
    }
    *sig_rl = strdup(sigrlstr.c_str());
    if (*sig_rl == nullptr) {
        return false;
    }
    *sig_rl_size = (uint32_t)sigrlstr.length();
    return true;
}

bool powServer::process_msg3(powSession* current, sgx_ra_msg3_t* msg3,
    ra_msg4_t& msg4, uint32_t quote_sz)
{

    if (CRYPTO_memcmp(&msg3->g_a, &current->msg1.g_a,
            sizeof(sgx_ec256_public_t))) {
        cerr << "msg1.ga != msg3.ga\n";
        return false;
    }
    sgx_mac_t msgMAC;
    cmac128(current->smk, (unsigned char*)&msg3->g_a, sizeof(sgx_ra_msg3_t) - sizeof(sgx_mac_t) + quote_sz, (unsigned char*)msgMAC);
    if (CRYPTO_memcmp(msg3->mac, msgMAC, sizeof(sgx_mac_t))) {
        cerr << "broken msg3 from client\n";
        return false;
    }
    char* b64quote;
    b64quote = base64_encode((char*)&msg3->quote, quote_sz);
    sgx_quote_t* q;
    q = (sgx_quote_t*)msg3->quote;
    if (memcmp(current->msg1.gid, &q->epid_group_id, sizeof(sgx_epid_group_id_t))) {
        cerr << "Attestation failed. Differ gid\n";
        return false;
    }
    if (get_attestation_report(b64quote, msg3->ps_sec_prop, &msg4)) {
        unsigned char vfy_rdata[64];
        unsigned char msg_rdata[144]; /* for Ga || Gb || VK */

        sgx_report_body_t* r = (sgx_report_body_t*)&q->report_body;

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

        cmac128(current->kdk, (unsigned char*)("\x01VK\x00\x80\x00"),
            6, current->vk);

        /* Build our plaintext */

        memcpy(msg_rdata, current->g_a, 64);
        memcpy(&msg_rdata[64], current->g_b, 64);
        memcpy(&msg_rdata[128], current->vk, 16);

        /* SHA-256 hash */

        sha256_digest(msg_rdata, 144, vfy_rdata);

        if (CRYPTO_memcmp((void*)vfy_rdata, (void*)&r->report_data, 64)) {
            cerr << "Report verification failed.\n";
            return false;
        }

        /*
         * A real service provider would validate that the pow_enclave
         * report is from an pow_enclave that they recognize. Namely,
         * that the MRSIGNER matches our signing key, and the MRENCLAVE
         * hash matches an pow_enclave that we compiled.
         *
         * Other policy decisions might include examining ISV_SVN to
         * prevent outdated/deprecated software from successfully
         * attesting, and ensuring the TCB is not out of date.
         */

        /*
         * If the pow_enclave is trusted, derive the MK and SK. Also get
         * SHA256 hashes of these so we can verify there's a shared
         * secret between us and the client.
         */

        if (msg4.status_) {

            cmac128(current->kdk, (unsigned char*)("\x01MK\x00\x80\x00"),
                6, current->mk);
            cmac128(current->kdk, (unsigned char*)("\x01SK\x00\x80\x00"),
                6, current->sk);

            current->enclaveTrusted = true;
        }
    } else {
        cerr << "Attestation failed\n";
        return false;
    }

    return true;
}

bool powServer::get_attestation_report(const char* b64quote, sgx_ps_sec_prop_desc_t secprop, ra_msg4_t* msg4)
{
    IAS_Request* req = nullptr;
    map<string, string> payload;
    vector<string> messages;
    ias_error_t status;
    string content;

    req = new IAS_Request(_ias, (uint16_t)_iasVersion);
    if (req == nullptr) {
        cerr << "Exception while creating IAS request object\n";
        return false;
    }

    payload.insert(make_pair("isvEnclaveQuote", b64quote));

    status = req->report(payload, content, messages);
    if (status == IAS_OK) {
        using namespace json;
        JSON reportObj = JSON::Load(content);

        /*
         * If the report returned a version number (API v3 and above), make
         * sure it matches the API version we used to fetch the report.
         *
         * For API v3 and up, this field MUST be in the report.
         */

        if (reportObj.hasKey("version")) {
            unsigned int rversion = (unsigned int)reportObj["version"].ToInt();
            if (_iasVersion != rversion) {
                cerr << "Report version " << rversion << " does not match API version " << _iasVersion << endl;
                return false;
            }
        }

        memset(msg4, 0, sizeof(ra_msg4_t));

        if (!(reportObj["isvEnclaveQuoteStatus"].ToString().compare("OK"))) {
            msg4->status_ = true;
        } else if (!(reportObj["isvEnclaveQuoteStatus"].ToString().compare("CONFIGURATION_NEEDED"))) {
            msg4->status_ = true;
        } else if (!(reportObj["isvEnclaveQuoteStatus"].ToString().compare("GROUP_OUT_OF_DATE"))) {
            msg4->status_ = true;
        } else {
            msg4->status_ = false;
        }
    }
    return true;
}

bool powServer::process_signedHash(powSession* session, powSignedHash req)
{
    string Mac, signature((char*)req.signature_, 16);
    cryptoObj_.cmac128(req.hash_, Mac, (u_char*)((const char*)session->sk), 16);
    if (Mac.compare(signature)) {
        cerr << "client signature unvalid" << endl;
        return false;
    }
    return true;
}

bool powServer::extractMQFromDataSR(EpollMessage_t& newMessage)
{
    return dataSRObj_->extractMQ2RAServer(newMessage);
}

bool powServer::insertMQToDataSR(EpollMessage_t& newMessage)
{
    return dataSRObj_->insertMQ2DataSR_CallBack(newMessage);
}

bool powServer::insertMQToDedupCore(EpollMessage_t& newMessage)
{
    return dataSRObj_->insertMQ2DedupCore(newMessage);
}