#include <bits/stdc++.h>
#include <boost/thread/thread.hpp>

#include "chunker.hpp"
#include "configure.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "keyClient.hpp"
#include "reciver.hpp"
#include "retriever.hpp"
#include "sender.hpp"
//TODO:refrac
#include "../pow/include/powClient.hpp"

using namespace std;

//MessageQueue<Chunk>mq1;
Configure config("config.json");
keyCache kCache;

chunker* Chunker;
keyClient* keyClientObj;
encoder* coder;
Sender* sender;
receiver* recver;
decoder* dcoder;
Retriever* retriever;
powClient* Pow;

void usage()
{
    cout << "client -r filename for receive file" << endl;
    cout << "client -s filename for send file" << endl;
}

int main(int argv, char* argc[])
{
    cerr << setbase(10);

    initMQForClient();

    vector<boost::thread*> thList;
    boost::thread* th;

    if (argv != 3) {
        usage();
        return 0;
    }

    if (strcmp("-r", argc[1]) == 0) {
        //run receive
        dcoder = new decoder();
        recver = new receiver();
        retriever = new Retriever(argc[2]);

        //start receiver thread
        recver->run(argc[2]);

        //start decoder thread
        dcoder->run();

        retriever->run();
    } else if (strcmp("-s", argc[1]) == 0) {
        //run send
        keyClientObj = new keyClient();
        Chunker = new chunker(argc[2], keyClientObj);
        coder = new encoder();
        sender = new Sender();
        Pow = new powClient();

        //start pow thread
        th = new boost::thread(boost::bind(&powClient::run, Pow));
        thList.push_back(th);

        //start chunking thread
        th = new boost::thread(boost::bind(&chunker::chunking, Chunker));
        thList.push_back(th);

        //start key client thread
        for (int i = 0; i < config.getKeyClientThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&keyClient::run, kex));
            thList.push_back(th);
        }

        //start encode thread
        for (int i = 0; i < config.getEncoderThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&encoder::run, coder));
            thList.push_back(th);
        }

        //start sender thread
        for (int i = 0; i < config.getSenderThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&Sender::run, sender));
            thList.push_back(th);
        }
    } else {
        usage();
        return 0;
    }

    for (auto it : thList) {
        it->join();
    }

    return 0;
}
