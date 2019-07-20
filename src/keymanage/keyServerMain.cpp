//
// Created by a on 11/17/18.
//

#define DEBUG

#include "Socket.hpp"
#include "keyServer.hpp"

Configure config("config.json");
util::keyCache kCache;

int main()
{
    Socket socket(SERVERTCP, "", config.getKeyServerPort());
    boost::thread* th;
    keyServer server;
    while (true) {
        Socket tmpSocket = socket.Listen();
        th = new boost::thread(boost::bind(&keyServer::run, &server, tmpSocket));
    }
    return 0;
}