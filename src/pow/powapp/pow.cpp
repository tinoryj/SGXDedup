//
// Created by a on 12/9/18.
//
#include "pow.hpp"
using namespace std;

POW::POW(string ip,int port){
    _inputMQ.createQueue("encoder to POW",READ_MESSAGE);
    _outputMQ.createQueue("POW to sender",WRITE_MESSAGE);
    sgx_status status;
    status=sgx_create_enclave("enclave.signed.so",SGX_DEBUG_FLAG, \
							&token,&updated,&global_eid,NULL);

    if ( status != SGX_SUCCESS ) {
        fprintf(stderr, "sgx_create_enclave: %s: %08x\n",
                ENCLAVE_NAME, status);
        if ( status == SGX_ERROR_ENCLAVE_FILE_ACCESS )
            fprintf(stderr, "Did you forget to set LD_LIBRARY_PATH?\n");
        return 1;
    }

    raProcess(_masterEnclaveID);
    _Socket.init(CLIENTTCP,ip,port);
}


void POW::run() {
    vector<Chunk>chunkList;
    vector<unsigned int>sendList;
    Chunk tmpChunk;
    char signature[128];
    string batchData,batchHash;
    char* dataHeader=new char[sizeof(int)];

    while(1){
        chunkList.clear();
        sendList.clear();
        batchData.clear();
        batchHash.clear();
        for(int i=0;i<_batchSize;i++){
            _inputMQ.pop(tmpChunk);
            chunkList.push_back(tmpChunk);
            int logicDataSize=tmpChunk.getLogicDataSize();
            memcpy(dataHeader,(char*)&logicDataSize,sizeof(int));
            for(int j=0;j<4;j++)batchData+=dataHeader[j];
            batchData+=tmpChunk.getLogicData();
            batchHash+=tmpChunk.getChunkHash();
        }
        ecall_calHash(_masterEnclaveID,context,batchData.c_str(),batchData.length(),signature);
        sendList=request(batchHash,signature);
        for(auto it:sendList){
            _outputMQ.push(chunkList[it]);
        }
    }
}
int POW::raProcess (sgx_enclave_id_t eid)
{
    sgx_status_t status, sgxrv, pse_status;
    sgx_ra_msg1_t msg1;
    sgx_ra_msg2_t *msg2 = NULL;
    sgx_ra_msg3_t *msg3 = NULL;
    ra_msg4_t *msg4 = NULL;
    uint32_t msg0_extended_epid_group_id = 0;
    uint32_t msg3_sz;
    uint32_t flags= config->flags;
    sgx_ra_context_t ra_ctx= 0xdeadbeef;
    int rv;
    MsgIO *msgio;
    size_t msg4sz = 0;
    int enclaveTrusted = NotTrusted; // Not Trusted
    int b_pse= OPT_ISSET(flags, OPT_PSE);

    msgio = new MsgIO();

    /* Executes an ECALL that runs sgx_ra_init() */

    if ( debug ) fprintf(stderr, "+++ using default public key\n");
    status= enclave_ra_init_def(eid, &sgxrv, b_pse, &ra_ctx,
                                &pse_status);

    /* Did the ECALL succeed? */
    if ( status != SGX_SUCCESS ) {
        fprintf(stderr, "enclave_ra_init: %08x\n", status);
        return 1;
    }

    /* If we asked for a PSE session, did that succeed? */
    if (b_pse) {
        if ( pse_status != SGX_SUCCESS ) {
            fprintf(stderr, "pse_session: %08x\n", sgxrv);
            return 1;
        }
    }

    /* Did sgx_ra_init() succeed? */
    if ( sgxrv != SGX_SUCCESS ) {
        fprintf(stderr, "sgx_ra_init: %08x\n", sgxrv);
        return 1;
    }

    /* Generate msg0 */

    status = sgx_get_extended_epid_group_id(&msg0_extended_epid_group_id);
    if ( status != SGX_SUCCESS ) {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "sgx_get_extended_epid_group_id: %08x\n", status);
        return 1;
    }
    if ( verbose ) {
        dividerWithText(stderr, "Msg0 Details");
        dividerWithText(fplog, "Msg0 Details");
        fprintf(stderr,   "Extended Epid Group ID: ");
        fprintf(fplog,   "Extended Epid Group ID: ");
        print_hexstring(stderr, &msg0_extended_epid_group_id,
                        sizeof(uint32_t));
        print_hexstring(fplog, &msg0_extended_epid_group_id,
                        sizeof(uint32_t));
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    /* Generate msg1 */

    status= sgx_ra_get_msg1(ra_ctx, eid, sgx_ra_get_ga, &msg1);
    if ( status != SGX_SUCCESS ) {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "sgx_ra_get_msg1: %08x\n", status);
        fprintf(fplog, "sgx_ra_get_msg1: %08x\n", status);
        return 1;
    }

    if ( verbose ) {
        dividerWithText(stderr,"Msg1 Details");
        dividerWithText(fplog,"Msg1 Details");
        fprintf(stderr,   "msg1.g_a.gx = ");
        fprintf(fplog,   "msg1.g_a.gx = ");
        print_hexstring(stderr, msg1.g_a.gx, 32);
        print_hexstring(fplog, msg1.g_a.gx, 32);
        fprintf(stderr, "\nmsg1.g_a.gy = ");
        fprintf(fplog, "\nmsg1.g_a.gy = ");
        print_hexstring(stderr, msg1.g_a.gy, 32);
        print_hexstring(fplog, msg1.g_a.gy, 32);
        fprintf(stderr, "\nmsg1.gid    = ");
        fprintf(fplog, "\nmsg1.gid    = ");
        print_hexstring(stderr, msg1.gid, 4);
        print_hexstring(fplog, msg1.gid, 4);
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    /*
     * Send msg0 and msg1 concatenated together (msg0||msg1). We do
     * this for efficiency, to eliminate an additional round-trip
     * between client and server. The assumption here is that most
     * clients have the correct extended_epid_group_id so it's
     * a waste to send msg0 separately when the probability of a
     * rejection is astronomically small.
     *
     * If it /is/ rejected, then the client has only wasted a tiny
     * amount of time generating keys that won't be used.
     */

//	dividerWithText(fplog, "Msg0||Msg1 ==> SP");
    fsend_msg_partial(fplog, &msg0_extended_epid_group_id,
                      sizeof(msg0_extended_epid_group_id));
    fsend_msg(fplog, &msg1, sizeof(msg1));
//	divider(fplog);

//	dividerWithText(stderr, "Copy/Paste Msg0||Msg1 Below to SP");
    msgio->send_partial(&msg0_extended_epid_group_id,
                        sizeof(msg0_extended_epid_group_id));
    msgio->send(&msg1, sizeof(msg1));
//	divider(stderr);

    fprintf(stderr, "Waiting for msg2\n");

    /* Read msg2
     *
     * msg2 is variable length b/c it includes the revocation list at
     * the end. msg2 is malloc'd in readZ_msg do free it when done.
     */

    rv= msgio->read((void **) &msg2, NULL);
    if ( rv == 0 ) {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "protocol error reading msg2\n");
        exit(1);
    } else if ( rv == -1 ) {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "system error occurred while reading msg2\n");
        exit(1);
    }

    if ( verbose ) {
        dividerWithText(stderr, "Msg2 Details");
        dividerWithText(fplog, "Msg2 Details (Received from SP)");
        fprintf(stderr,   "msg2.g_b.gx      = ");
        fprintf(fplog,   "msg2.g_b.gx      = ");
        print_hexstring(stderr, &msg2->g_b.gx, sizeof(msg2->g_b.gx));
        print_hexstring(fplog, &msg2->g_b.gx, sizeof(msg2->g_b.gx));
        fprintf(stderr, "\nmsg2.g_b.gy      = ");
        fprintf(fplog, "\nmsg2.g_b.gy      = ");
        print_hexstring(stderr, &msg2->g_b.gy, sizeof(msg2->g_b.gy));
        print_hexstring(fplog, &msg2->g_b.gy, sizeof(msg2->g_b.gy));
        fprintf(stderr, "\nmsg2.spid        = ");
        fprintf(fplog, "\nmsg2.spid        = ");
        print_hexstring(stderr, &msg2->spid, sizeof(msg2->spid));
        print_hexstring(fplog, &msg2->spid, sizeof(msg2->spid));
        fprintf(stderr, "\nmsg2.quote_type  = ");
        fprintf(fplog, "\nmsg2.quote_type  = ");
        print_hexstring(stderr, &msg2->quote_type, sizeof(msg2->quote_type));
        print_hexstring(fplog, &msg2->quote_type, sizeof(msg2->quote_type));
        fprintf(stderr, "\nmsg2.kdf_id      = ");
        fprintf(fplog, "\nmsg2.kdf_id      = ");
        print_hexstring(stderr, &msg2->kdf_id, sizeof(msg2->kdf_id));
        print_hexstring(fplog, &msg2->kdf_id, sizeof(msg2->kdf_id));
        fprintf(stderr, "\nmsg2.sign_ga_gb  = ");
        fprintf(fplog, "\nmsg2.sign_ga_gb  = ");
        print_hexstring(stderr, &msg2->sign_gb_ga, sizeof(msg2->sign_gb_ga));
        print_hexstring(fplog, &msg2->sign_gb_ga, sizeof(msg2->sign_gb_ga));
        fprintf(stderr, "\nmsg2.mac         = ");
        fprintf(fplog, "\nmsg2.mac         = ");
        print_hexstring(stderr, &msg2->mac, sizeof(msg2->mac));
        print_hexstring(fplog, &msg2->mac, sizeof(msg2->mac));
        fprintf(stderr, "\nmsg2.sig_rl_size = ");
        fprintf(fplog, "\nmsg2.sig_rl_size = ");
        print_hexstring(stderr, &msg2->sig_rl_size, sizeof(msg2->sig_rl_size));
        print_hexstring(fplog, &msg2->sig_rl_size, sizeof(msg2->sig_rl_size));
        fprintf(stderr, "\nmsg2.sig_rl      = ");
        fprintf(fplog, "\nmsg2.sig_rl      = ");
        print_hexstring(stderr, &msg2->sig_rl, msg2->sig_rl_size);
        print_hexstring(fplog, &msg2->sig_rl, msg2->sig_rl_size);
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

    if ( debug ) {
        fprintf(stderr, "+++ msg2_size = %zu\n",
                sizeof(sgx_ra_msg2_t)+msg2->sig_rl_size);
        fprintf(fplog, "+++ msg2_size = %zu\n",
                sizeof(sgx_ra_msg2_t)+msg2->sig_rl_size);
    }

    /* Process Msg2, Get Msg3  */
    /* object msg3 is malloc'd by SGX SDK, so remember to free when finished */

    msg3 = NULL;

    status = sgx_ra_proc_msg2(ra_ctx, eid,
                              sgx_ra_proc_msg2_trusted, sgx_ra_get_msg3_trusted, msg2,
                              sizeof(sgx_ra_msg2_t) + msg2->sig_rl_size,
                              &msg3, &msg3_sz);

    if ( msg2 ) {
        free(msg2);
        msg2 = NULL;
    }

    if ( status != SGX_SUCCESS ) {
        enclave_ra_close(eid, &sgxrv, ra_ctx);
        fprintf(stderr, "sgx_ra_proc_msg2: %08x\n", status);
        fprintf(fplog, "sgx_ra_proc_msg2: %08x\n", status);

        return 1;
    }

    if ( debug ) {
        fprintf(stderr, "+++ msg3_size = %u\n", msg3_sz);
        fprintf(fplog, "+++ msg3_size = %u\n", msg3_sz);
    }

    if ( verbose ) {
        dividerWithText(stderr, "Msg3 Details");
        dividerWithText(fplog, "Msg3 Details");
        fprintf(stderr,   "msg3.mac         = ");
        fprintf(fplog,   "msg3.mac         = ");
        print_hexstring(stderr, msg3->mac, sizeof(msg3->mac));
        print_hexstring(fplog, msg3->mac, sizeof(msg3->mac));
        fprintf(stderr, "\nmsg3.g_a.gx      = ");
        fprintf(fplog, "\nmsg3.g_a.gx      = ");
        print_hexstring(stderr, msg3->g_a.gx, sizeof(msg3->g_a.gx));
        print_hexstring(fplog, msg3->g_a.gx, sizeof(msg3->g_a.gx));
        fprintf(stderr, "\nmsg3.g_a.gy      = ");
        fprintf(fplog, "\nmsg3.g_a.gy      = ");
        print_hexstring(stderr, msg3->g_a.gy, sizeof(msg3->g_a.gy));
        print_hexstring(fplog, msg3->g_a.gy, sizeof(msg3->g_a.gy));
        fprintf(stderr, "\nmsg3.ps_sec_prop.sgx_ps_sec_prop_desc = ");
        fprintf(fplog, "\nmsg3.ps_sec_prop.sgx_ps_sec_prop_desc = ");
        print_hexstring(stderr, msg3->ps_sec_prop.sgx_ps_sec_prop_desc,
                        sizeof(msg3->ps_sec_prop.sgx_ps_sec_prop_desc));
        print_hexstring(fplog, msg3->ps_sec_prop.sgx_ps_sec_prop_desc,
                        sizeof(msg3->ps_sec_prop.sgx_ps_sec_prop_desc));
        fprintf(fplog, "\n");
        fprintf(stderr, "\nmsg3.quote       = ");
        fprintf(fplog, "\nmsg3.quote       = ");
        print_hexstring(stderr, msg3->quote, msg3_sz-sizeof(sgx_ra_msg3_t));
        print_hexstring(fplog, msg3->quote, msg3_sz-sizeof(sgx_ra_msg3_t));
        fprintf(fplog, "\n");
        fprintf(stderr, "\n");
        fprintf(fplog, "\n");
        divider(stderr);
        divider(fplog);
    }

//	dividerWithText(stderr, "Copy/Paste Msg3 Below to SP");
    msgio->send(msg3, msg3_sz);
//	divider(stderr);

//	dividerWithText(fplog, "Msg3 ==> SP");
    fsend_msg(fplog, msg3, msg3_sz);
//	divider(fplog);

    if ( msg3 ) {
        free(msg3);
        msg3 = NULL;
    }

    /* Read Msg4 provided by Service Provider, then process */

    msgio->read((void **)&msg4, &msg4sz);

//	edividerWithText("Enclave Trust Status from Service Provider");

    enclaveTrusted= msg4->status;
    if ( enclaveTrusted == Trusted ) {
        eprintf("Enclave TRUSTED\n");
    }
    else if ( enclaveTrusted == NotTrusted ) {
        eprintf("Enclave NOT TRUSTED\n");
    }
    else if ( enclaveTrusted == Trusted_ItsComplicated ) {
        // Trusted, but client may be untrusted in the future unless it
        // takes action.

        eprintf("Enclave Trust is TRUSTED and COMPLICATED. The client is out of date and\nmay not be trusted in the future depending on the service provider's  policy.\n");
    } else {
        // Not Trusted, but client may be able to take action to become
        // trusted.

        eprintf("Enclave Trust is NOT TRUSTED and COMPLICATED. The client is out of date.\n");
    }

    /* check to see if we have a PIB by comparing to empty PIB */
    sgx_platform_info_t emptyPIB;
    memset(&emptyPIB, 0, sizeof (sgx_platform_info_t));

    int retPibCmp = memcmp(&emptyPIB, (void *)(&msg4->platformInfoBlob), sizeof (sgx_platform_info_t));

    if (retPibCmp == 0 ) {
        if ( verbose ) eprintf("A Platform Info Blob (PIB) was NOT provided by the IAS\n");
    } else {
        if ( verbose ) eprintf("A Platform Info Blob (PIB) was provided by the IAS\n");

        if ( debug )  {
            eprintf("+++ PIB: " );
            print_hexstring(stderr, &msg4->platformInfoBlob, sizeof (sgx_platform_info_t));
            print_hexstring(fplog, &msg4->platformInfoBlob, sizeof (sgx_platform_info_t));
            eprintf("\n");
        }

        /* We have a PIB, so check to see if there are actions to take */
        sgx_update_info_bit_t update_info;
        sgx_status_t ret = sgx_report_attestation_status(&msg4->platformInfoBlob,
                                                         enclaveTrusted, &update_info);

        if ( debug )  eprintf("+++ sgx_report_attestation_status ret = 0x%04x\n", ret);

        edivider();

        /* Check to see if there is an update needed */
        if ( ret == SGX_ERROR_UPDATE_NEEDED ) {

            edividerWithText("Platform Update Required");
            eprintf("The following Platform Update(s) are required to bring this\n");
            eprintf("platform's Trusted Computing Base (TCB) back into compliance:\n\n");
            if( update_info.pswUpdate ) {
                eprintf("  * Intel SGX Platform Software needs to be updated to the latest version.\n");
            }

            if( update_info.csmeFwUpdate ) {
                eprintf("  * The Intel Management Engine Firmware Needs to be Updated.  Contact your\n");
                eprintf("    OEM for a BIOS Update.\n");
            }

            if( update_info.ucodeUpdate )  {
                eprintf("  * The CPU Microcode needs to be updated.  Contact your OEM for a platform\n");
                eprintf("    BIOS Update.\n");
            }
            eprintf("\n");
            edivider();
        }
    }

    /*
     * If the enclave is trusted, fetch a hash of the the MK and SK from
     * the enclave to show proof of a shared secret with the service
     * provider.
     */

    if ( enclaveTrusted == Trusted ) {
        sgx_status_t key_status, sha_status;
        sgx_sha256_hash_t mkhash, skhash;

        // First the MK

        if ( debug ) eprintf("+++ fetching SHA256(MK)\n");
        status= enclave_ra_get_key_hash(eid, &sha_status, &key_status, ra_ctx,
                                        SGX_RA_KEY_MK, &mkhash);
        if ( debug ) eprintf("+++ ECALL enclage_ra_get_key_hash (MK) ret= 0x%04x\n",
                             status);

        if ( debug ) eprintf("+++ sgx_ra_get_keys (MK) ret= 0x%04x\n", key_status);
        // Then the SK

        if ( debug ) eprintf("+++ fetching SHA256(SK)\n");
        status= enclave_ra_get_key_hash(eid, &sha_status, &key_status, ra_ctx,
                                        SGX_RA_KEY_SK, &skhash);
        if ( debug ) eprintf("+++ ECALL enclage_ra_get_key_hash (MK) ret= 0x%04x\n",
                             status);

        if ( debug ) eprintf("+++ sgx_ra_get_keys (MK) ret= 0x%04x\n", key_status);
        if ( verbose ) {
            eprintf("SHA256(MK) = ");
            print_hexstring(stderr, mkhash, sizeof(mkhash));
            print_hexstring(fplog, mkhash, sizeof(mkhash));
            eprintf("\n");
            eprintf("SHA256(SK) = ");
            print_hexstring(stderr, skhash, sizeof(skhash));
            print_hexstring(fplog, skhash, sizeof(skhash));
            eprintf("\n");
        }
    }

    if ( msg4 ) {
        free (msg4);
        msg4 = NULL;
    }

    enclave_ra_close(eid, &sgxrv, ra_ctx);

    return 0;
}

vector<unsigned int> POW::request(string hashList, string signature) {
    vector<unsigned int>ans;
    string buffer;
    _Socket.Send(hashList);
    _Socket.Send(signature);
    _Socket.Recv(buffer);

    int len=buffer.length();
    for(int i=0;i<len;i+=sizeof(int)){
        unsigned int ind=0;
        for(int j=i;j<i+sizeof(int);j++){
            ind<<=8;
            unsigned char c=buffer[i];
            ind|=(unsigned int)c;
        }
        ans.push_back(ind);
    }
    return ans;
}