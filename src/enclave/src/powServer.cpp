#include "powServer.hpp"
#include <sgx_uae_epid.h>
#include <sgx_uae_launch.h>
#include <sgx_uae_quote_ex.h>
//./sp -s 928A6B0E3CDDAD56EB3BADAA3B63F71F -A ./client.crt
// -C ./client.crt --ias-cert-key=./client.pem -x -d -v
// -A AttestationReportSigningCACert.pem -C client.crt
// -s 797F0D90EE75B24B554A73AB01FD3335 -Y client.pem

void PRINT_BYTE_ARRAY_POW_SERVER(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

void powServer::closeSession(int fd)
{
    sessions.erase(fd);
}

powServer::powServer()
{
    cryptoObj_ = new CryptoPrimitive();
}

powServer::~powServer()
{
    auto it = sessions.begin();
    while (it != sessions.end()) {
        delete it->second;
        it++;
    }
    sessions.clear();
    delete cryptoObj_;
}

bool powServer::process_signedHash(enclaveSession* session, u_char* mac, u_char* hashList, int chunkNumber)
{
    u_char serverMac[16];
    cryptoObj_->cmac128(hashList, chunkNumber, serverMac, session->sk, 16);
    if (memcmp(mac, serverMac, 16) == 0) {
        return true;
    } else {
#if SYSTEM_BREAK_DOWN == 1
        return true;
#endif
        cerr << "PowServer : client signature unvalid, client mac = " << endl;
        PRINT_BYTE_ARRAY_POW_SERVER(stderr, mac, 16);
        cerr << "\t server mac = " << endl;
        PRINT_BYTE_ARRAY_POW_SERVER(stderr, serverMac, 16);
        return false;
    }
}
