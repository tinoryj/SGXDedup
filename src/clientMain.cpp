/*
Usage:
	app chunking_file configure_file
*/

#define DEBUG

#ifdef DEBUG
	#include<unistd.h>
#endif

#include "configure.hpp"
#include "chunk.hpp"
#include "_chunker.hpp"
#include "chunker.hpp"

#include <iostream>
#include <fstream>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>

using namespace std;

//MessageQueue<Chunk>mq1;
chunker *Chunker;
Configure config;
keyClient *kex;
encoder *coder;

int main(int argv, char *argc[]) {
    config.readConf(argc[2]);

    Chunker=new chunker(argc[1]);
    kex=new keyClient();
    coder=new encoder();

    #ifdef DEBUG
    	clock_t end,stat=clock();
    	double dur,per=CLOCKS_PER_SEC;
    #endif

    boost::thread th(boost::bind(&chunker::chunking,Chunker));
    th.detach();
    
    for(int i=0;i<config.getMaxThreadLimit();i++){
        boost::thread th(boost::bind(&keyClient::run,kex));
        th.detach();
    }

    for(int i=0;i<config,getMaxThreadLimit();i++){
        boost::thread th(boost::bind(&encoder::run,coder));
        th.detach();
    }


    #ifdef DEBUG
    	end=clock();
    	dur=end-stat;
    	cout<<"time: "<<dur/per<<endl;
    #endif

//	boost::thread th1(chunker->chunking);
//	th1.detach();
    return 0;
}