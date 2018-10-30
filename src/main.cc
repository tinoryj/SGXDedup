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
_Chunker *Chunker;
Configure config;

int main(int argv, char *argc[]) {
    config.readConf(argc[2]);

    Chunker=new chunker(argc[1]);

    #ifdef DEBUG
    	clock_t end,stat=clock();
    	double dur,per=CLOCKS_PER_SEC;
    #endif

    Chunker->chunking();

    #ifdef DEBUG
    	end=clock();
    	dur=end-stat;
    	cout<<"time: "<<dur/per<<endl;
    #endif

//	boost::thread th1(chunker->chunking);
//	th1.detach();
    return 0;
}