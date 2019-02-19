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

#include <iostream>
#include <fstream>
#include <string>

#include <boost/thread/thread.hpp>

#include "configure.hpp"
#include "chunker.hpp"
#include "keyClient.hpp"
#include "encoder.hpp"
#include "sender.hpp"
#include "retriever.hpp"
#include "reciver.hpp"
#include "decoder.hpp"
#include "../pow/include/powClient.hpp"


using namespace std;

//MessageQueue<Chunk>mq1;
Configure config("config.json");
util::keyCache kCache;

chunker *Chunker;
keyClient *kex;
encoder *coder;
Sender *sender;
receiver *recver;
decoder *dcoder;
Retriever *retriever;
powClient *Pow;



void usage(){
    cout<<"client -r filename for receive file\n";
    cout<<"client -s filename for send file\n";
}

int main(int argv, char *argc[]) {

    initMQForClient();


    vector<boost::thread*>thList;
    boost::thread *th;

    if(argv!=3){
        usage();
        return 0;
    }

    if(strcmp("-r",argc[1])==0){
        //run receive
        dcoder=new decoder();
        recver=new receiver();
        retriever=new Retriever(argc[2]);

        //start receiver thread
        recver->run(argc[2]);

        //start decoder thread
        dcoder->run();
    }
    else if(strcmp("-s",argc[1])==0){
        //run send
        Chunker=new chunker(argc[2]);
        kex=new keyClient();
        coder=new encoder();
        sender=new Sender();
        Pow=new powClient();

        //start pow thread
        th=new boost::thread(boost::bind(&powClient::run,Pow));
        thList.push_back(th);


        //start chunking thread
        th=new boost::thread(boost::bind(&chunker::chunking,Chunker));
        thList.push_back(th);

        //start key client thread
        for(int i=0;i<config.getKeyClientThreadLimit();i++){
            th=new boost::thread(boost::bind(&keyClient::run,kex));
            thList.push_back(th);
        }

        //start encode thread
        for(int i=0;i<config.getEncoderThreadLimit();i++){
            th=new boost::thread(boost::bind(&encoder::run,coder));
            thList.push_back(th);
        }

        //start sender thread
        for(int i=0;i<config.getSenderThreadLimit();i++){
            th=new boost::thread(boost::bind(&Sender::run,sender));
            thList.push_back(th);
        }
    }
    else{
        usage();
        return 0;
    }

    for(auto it:thList){
        it->join();
    }
    while(1){}

    return 0;
}
