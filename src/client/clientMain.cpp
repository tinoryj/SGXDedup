#include "../pow/include/powClient.hpp"
#include "chunker.hpp"
#include "configure.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "keyClient.hpp"
#include "reciver.hpp"
#include "retriever.hpp"
#include "sender.hpp"
#include <bits/stdc++.h>
#include <boost/thread/thread.hpp>

using namespace std;

Configure config("config.json");
keyCache kCache;
Chunker* chunkerObj;
keyClient* keyClientObj;
encoder* encoderObj;
Sender* senderObj;
receiver* recverObj;
decoder* dcoderObj;
Retriever* retrieverObj;
powClient* PowClientObj;

void usage()
{
    cout << "client -r filename for receive file" << endl;
    cout << "client -s filename for send file" << endl;
}

int main(int argv, char* argc[])
{
    vector<boost::thread*> thList;
    boost::thread* th;

    if (argv != 3) {
        usage();
        return 0;
    }

    if (strcmp("-r", argc[1]) == 0) {
        //run receive
        dcoderObj = new decoder();
        recverObj = new receiver();
        retrieverObj = new Retriever(argc[2]);
        //start receiver thread
        recverObj->run(argc[2]);
        //start decoder thread
        dcoderObj->run();
        retrieverObj->run();

    } else if (strcmp("-s", argc[1]) == 0) {
        //run send
        senderObj = new Sender();
        PowClientObj = new powClient(senderObj);
        encoderObj = new encoder(PowClientObj);
        keyClientObj = new keyClient(encoderObj);
        chunkerObj = new Chunker(argc[2], keyClientObj);

        //start pow thread
        th = new boost::thread(boost::bind(&powClient::run, PowClientObj));
        thList.push_back(th);

        //start chunking thread
        th = new boost::thread(boost::bind(&Chunker::chunking, chunkerObj));
        thList.push_back(th);

        //start key client thread
        for (int i = 0; i < config.getKeyClientThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&keyClient::run, keyClientObj));
            thList.push_back(th);
        }

        //start encode thread
        for (int i = 0; i < config.getEncoderThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&encoder::run, encoderObj));
            thList.push_back(th);
        }

        //start sender thread
        for (int i = 0; i < config.getSenderThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&Sender::run, senderObj));
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
