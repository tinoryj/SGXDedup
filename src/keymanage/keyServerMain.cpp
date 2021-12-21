#include "keyServer.hpp"
#include <signal.h>

Configure config("config.json");
keyServer* server;

struct timeval timestartKeyServerMain;
struct timeval timeendKeyServerMain;

void CTRLC(int s)
{
    cerr << "KeyManager exit with keyboard interrupt" << endl;
    delete server;
    exit(0);
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    sa.sa_handler = CTRLC;
    sigaction(SIGKILL, &sa, 0);
    sigaction(SIGINT, &sa, 0);

    ssl* keySecurityChannelTemp = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), SERVERSIDE);
    boost::thread* th;
    server = new keyServer(keySecurityChannelTemp);
    cerr << "KeyServerMain : key server remote attestation done, start provide service" << endl;
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
    th = new boost::thread(boost::bind(&keyServer::runCTRModeMaskGenerate, server));
    th->detach();
#endif
    while (true) {
        SSL* sslConnection = keySecurityChannelTemp->sslListen().second;
        th = new boost::thread(boost::bind(&keyServer::runKeyGenerateThread, server, sslConnection));
        th->detach();
    }
    return 0;
}