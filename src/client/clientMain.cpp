#include "../pow/include/powClient.hpp"
#include "chunker.hpp"
#include "configure.hpp"
#include "encoder.hpp"
#include "keyClient.hpp"
#include "recvDecode.hpp"
#include "retriever.hpp"
#include "sender.hpp"
#include <bits/stdc++.h>
#include <boost/thread/thread.hpp>

using namespace std;

Configure config("config.json");
KeyCache kCache;
Chunker* chunkerObj;
keyClient* keyClientObj;
Encoder* encoderObj;
Sender* senderObj;
RecvDecode* recvDecodeObj;
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
        string fileName(argc[2]);

        recvDecodeObj = new RecvDecode(fileName);
        //Recipe_t fileRecipe = recvDecodeObj->getFileRecipeHead();
        //memcpy(&fileRecipe, &recvDecodeObj->getFileRecipeHead(), sizeof(Recipe_t));
        retrieverObj = new Retriever(fileName, recvDecodeObj);

        for (int i = 0; i < config.getRecvDecodeThreadLimit(); i++) {
            th = new boost::thread(boost::bind(&RecvDecode::run, recvDecodeObj));
            thList.push_back(th);
        };
        //start decoder thread
        retrieverObj->run();

    } else if (strcmp("-s", argc[1]) == 0) {
        //run send
        senderObj = new Sender();
        PowClientObj = new powClient(senderObj);
        encoderObj = new Encoder(PowClientObj);
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
            th = new boost::thread(boost::bind(&Encoder::run, encoderObj));
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
