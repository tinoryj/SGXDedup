#include "keyServer.hpp"

Configure config("config.json");

int main()
{
    ssl* keySecurityChannelTemp = new ssl(config.getKeyServerIP(), config.getKeyServerPort(), SERVERSIDE);
    boost::thread* th;
    keyServer* server = new keyServer(keySecurityChannelTemp);
    th = new boost::thread(boost::bind(&keyServer::runRA, server));
    th->detach();
    th = new boost::thread(boost::bind(&keyServer::runRAwithSPRequest, server));
    th->detach();
    while (true) {
        SSL* sslConnection = keySecurityChannelTemp->sslListen().second;
        th = new boost::thread(boost::bind(&keyServer::run, server, sslConnection));
        th->detach();
    }
    return 0;
}