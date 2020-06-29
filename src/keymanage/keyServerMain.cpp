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
    cout << "KeyServerMain : setup keyserver object done" << endl;
#if KEY_GEN_SGX_CFB == 1
    th = new boost::thread(boost::bind(&keyServer::runRA, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runRAwithSPRequest, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runSessionKeyUpdate, server));
    th->detach();
#elif KEY_GEN_SGX_CTR == 1
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
        if (server->getRASetupFlag()) {
            break;
        }
    }
    cout << "KeyServerMain : key server remote attestation done, start provide service" << endl;
#if KEY_GEN_EPOLL_MODE == 1
    th = new boost::thread(boost::bind(&keyServer::runRecvThread, server));
    th->detach();
    cout << "KeyServerMain : start epoll recv/send thread" << endl;
    th = new boost::thread(boost::bind(&keyServer::runSendThread, server));
    th->detach();
    cout << "KeyServerMain : start epoll collection thread" << endl;
    for (int i = 0; i < config.getKeyEnclaveThreadNumber(); i++) {
        th = new boost::thread(boost::bind(&keyServer::runKeyGenerateRequestThread, server, i));
        th->detach();
        cout << "KeyServerMain : start epoll key generate thread " << i << endl;
    }
    while (true)
        ;
#else
    while (true) {
        SSL* sslConnection = keySecurityChannelTemp->sslListen().second;
        th = new boost::thread(boost::bind(&keyServer::runKeyGenerateThread, server, sslConnection));
        th->detach();
    }
#endif
    return 0;
}