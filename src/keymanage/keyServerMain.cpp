#include "keyServer.hpp"
#include <signal.h>

Configure config("config.json");
keyServer* server;
void CTRLC(int s)
{
    cerr << "Key Manager close" << endl;
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
#if KEY_GEN_SGX_CFB == 1
    th = new boost::thread(boost::bind(&keyServer::runRA, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runRAwithSPRequest, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runSessionKeyUpdate, server));
    th->detach();
#elif SGX_KEY_GEN_CTR == 1
    th = new boost::thread(boost::bind(&keyServer::runRA, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runRAwithSPRequest, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runSessionKeyUpdate, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runCTRModeMaskGenerate, server));
    th->detach();
#endif
    while (true) {
        SSL* sslConnection = keySecurityChannelTemp->sslListen().second;
        th = new boost::thread(boost::bind(&keyServer::run, server, sslConnection));
        th->detach();
    }
    return 0;
}