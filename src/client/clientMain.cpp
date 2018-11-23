//
// Created by a on 11/17/18.
//

/*
Usage:
	app chunking_file configure_file
*/

#define DEBUG

#ifdef DEBUG
#include<unistd.h>
#endif

#include "configure.hpp"
#include "_chunker.hpp"
#include "chunker.hpp"
#include "keyClient.hpp"
#include "encoder.hpp"

#include <iostream>
#include <fstream>
#include <string>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/compute/detail/lru_cache.hpp>

using namespace std;

//MessageQueue<Chunk>mq1;
chunker *Chunker;
Configure config;
keyClient *kex;
encoder *coder;
boost::compute::detail::lru_cache<string,string>*kCache;

int main(int argv, char *argc[]) {
    config.readConf(argc[2]);

    Chunker=new chunker(argc[1]);
    kex=new keyClient();
    coder=new encoder();
    kCache=new boost::compute::detail::lru_cache<string,string>(config.getKeyCacheSize());

#ifdef DEBUG
    clock_t end,stat=clock();
    double dur,per=CLOCKS_PER_SEC;
#endif

    boost::thread th(boost::bind(&chunker::chunking,Chunker));
    th.detach();

    for(int i=0;i<config.getMaxThreadLimits();i++){
        boost::thread th(boost::bind(&keyClient::run,kex));
        th.detach();
    }

    for(int i=0;i<config.getMaxThreadLimits();i++){
        boost::thread th(boost::bind(&encoder::run,coder));
        th.detach();
    }


#ifdef DEBUG
    end=clock();
    dur=end-stat;
    cout<<"time: "<<dur/per<<endl;
#endif
//
//	boost::thread th1(Chunker->chunking);
//	th1.detach();
    return 0;
}