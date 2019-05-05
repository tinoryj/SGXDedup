//
// Created by a on 11/17/18.
//

#define DEBUG

#include "keyServer.hpp"
#include "_messageQueue.hpp"

Configure config("config.json");
util::keyCache kCache;

//argc[1] : config file name
int main(int argv,char** argc){
    initMQForKeyServer();

    config.readConf(argc[1]);

    keyServer ks;
    vector<boost::thread*>thList;
    boost::thread* th;

    //keyServer start to recv keyGen request

    for(int i=0;i<config.getKeyServerThreadLimit();i++){
        th=new boost::thread(boost::bind(&keyServer::runRecv,&ks));
        thList.push_back(th);
    }

    //start keyServer keyGen thread
    for(int i=0;i<1;i++){
        th=new boost::thread(boost::bind(&keyServer::runKeyGen,&ks));
        thList.push_back(th);
    }

    for(auto it:thList){
        it->join();
    }
    return 0;
}