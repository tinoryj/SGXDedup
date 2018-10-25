/*
Usage:
	app chunking_file configure_file
*/

#define DEBUG

#ifdef DEBUG
	#include<unistd.h>
#endif

#include "SimpleChunker.hpp"
#include "RabinChunker.hpp"
#include "configure.hpp"

#include <iostream>
#include <fstream>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>

using namespace std;

//MessageQueue<Chunk>mq1;
_Chunker *chunker;
Configure config;

int main(int argv, char *argc[]) {
    config.readConf(argc[2]);

    if (config.getChunkerType() == SIMPLE_CHUNKER) {
        chunker = new SimpleChunker(argc[1]);
    } else {
        chunker = new RabinChunker(argc[1]);
    }

    #ifdef DEBUG
    	clock_t end,stat=clock();
    	double dur,per=CLOCKS_PER_SEC;
    #endif

    chunker->chunking();

    #ifdef DEBUG
    	end=clock();
    	dur=end-stat;
    	cout<<"time: "<<dur/per<<endl;
    #endif

//	boost::thread th1(chunker->chunking);
//	th1.detach();
    return 0;
}