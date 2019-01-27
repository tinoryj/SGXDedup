//
// Created by a on 11/17/18.
//

/*
Usage:
	app chunking_file configure_file
*/

//may have bug when last mq did not remove from system

#define DEBUG

#ifdef DEBUG
#include<unistd.h>
#endif

#include "configure.hpp"
#include "_chunker.hpp"
#include "chunker.hpp"
#include "keyClient.hpp"
#include "encoder.hpp"
#include "cache.hpp"

#include <iostream>
#include <fstream>
#include <string>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>

using namespace std;

//MessageQueue<Chunk>mq1;
Configure config("config.json");
util::keyCache kCache;

chunker *Chunker;
keyClient *kex;
encoder *coder;



//argc[1]: file path
//argc[2]: config file path
int main(int argv, char *argc[]) {

    vector<boost::thread*>thList;
    boost::thread *th;
    config.readConf(argc[2]);

    Chunker=new chunker(argc[1]);
    kex=new keyClient();
    coder=new encoder();

    Chunker->chunking();
    kex->run();
    /*
    //start chunking thread
    th=new boost::thread(boost::bind(&chunker::chunking,Chunker));
    thList.push_back(th);

    config.getKeyClientThreadLimit();


    //start key client thread
    for(int i=0;i<config.getKeyClientThreadLimit();i++){
        th=new boost::thread(boost::bind(&keyClient::run,kex));
        thList.push_back(th);
    }

    //start encode thread
    for(int i=0;i<config.getEncoderThreadLimit();i++){
        th=new boost::thread(boost::bind(&encoder::run,coder));
        //thList.push_back(th);
    }


    for(auto it:thList){
        it->join();
    }
*/
    return 0;
}