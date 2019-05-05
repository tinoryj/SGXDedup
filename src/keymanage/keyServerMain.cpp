//
// Created by a on 11/17/18.
//

#define DEBUG

#include "keyServer.hpp"
#include "Socket.hpp"

Configure config("config.json");
util::keyCache kCache;

//argc[1] : config file name
int main(int argv,char** argc){
    Socket socket(SERVERTCP,"",0);
    boost::thread *th;
    keyServer server;
    while (1){
        Socket tmpSocket=socket.Listen();
        th=new boost::thread(boost::bind(&keyServer::run,&server,tmpSocket));
    }
    return 0;
}