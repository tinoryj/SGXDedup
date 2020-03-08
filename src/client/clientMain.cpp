#include "../pow/include/powClient.hpp"
#include "chunker.hpp"
#include "configure.hpp"
#include "encoder.hpp"
#include "keyClient.hpp"
#include "recvDecode.hpp"
#include "retriever.hpp"
#include "sender.hpp"
#include "sys/time.h"
#include <bits/stdc++.h>
#include <boost/thread/thread.hpp>
#include <signal.h>

using namespace std;

Configure config("config.json");
Chunker* chunkerObj;
keyClient* keyClientObj;
Sender* senderObj;
RecvDecode* recvDecodeObj;
Retriever* retrieverObj;
powClient* powClientObj;
Encoder* encoderObj;

struct timeval timestart;
struct timeval timeend;

void CTRLC(int s)
{
    cerr << "Client close" << endl;
    delete chunkerObj;
    delete keyClientObj;
    delete senderObj;
    delete recvDecodeObj;
    delete retrieverObj;
    delete powClientObj;
    delete encoderObj;
    exit(0);
}

void usage()
{
    cout << "[client -r filename] for receive file" << endl;
    cout << "[client -s filename] for send file" << endl;
}

int main(int argv, char* argc[])
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    sa.sa_handler = CTRLC;
    sigaction(SIGKILL, &sa, 0);
    sigaction(SIGINT, &sa, 0);

    vector<boost::thread*> thList;
    boost::thread* th;
    if (argv != 3) {
        usage();
        return 0;
    }
    boost::thread::attributes attrs;
    attrs.set_stack_size(200 * 1024 * 1024);

    if (strcmp("-r", argc[1]) == 0) {
        string fileName(argc[2]);

        recvDecodeObj = new RecvDecode(fileName);
        retrieverObj = new Retriever(fileName, recvDecodeObj);

        gettimeofday(&timestart, NULL);

        for (int i = 0; i < config.getRecvDecodeThreadLimit(); i++) {
            th = new boost::thread(attrs, boost::bind(&RecvDecode::run, recvDecodeObj));
            thList.push_back(th);
        }
        th = new boost::thread(attrs, boost::bind(&Retriever::recvThread, retrieverObj));
        thList.push_back(th);

    } else if (strcmp("-s", argc[1]) == 0) {

        senderObj = new Sender();

        gettimeofday(&timestart, NULL);
        powClientObj = new powClient(senderObj);
        gettimeofday(&timeend, NULL);
        long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        double second = diff / 1000000.0;
        cout << "System : init time is " << second << " s" << endl;
        gettimeofday(&timestart, NULL);

        u_char sessionKey[16];
        if (!senderObj->getKeyServerSK(sessionKey)) {
            cerr << "Client : get key server session key failed" << endl;
            delete senderObj;
            delete powClientObj;
            return 0;
        }
        encoderObj = new Encoder(powClientObj);
        keyClientObj = new keyClient(encoderObj, sessionKey);
        string inputFile(argc[2]);
        chunkerObj = new Chunker(inputFile, keyClientObj);

        //start chunking thread
        th = new boost::thread(attrs, boost::bind(&Chunker::chunking, chunkerObj));
        thList.push_back(th);

        //start key client thread
        th = new boost::thread(attrs, boost::bind(&keyClient::run, keyClientObj));
        thList.push_back(th);

        //start encoder thread
        th = new boost::thread(attrs, boost::bind(&Encoder::run, encoderObj));
        thList.push_back(th);
        //start pow thread
        th = new boost::thread(attrs, boost::bind(&powClient::run, powClientObj));
        thList.push_back(th);

        //start sender thread
        th = new boost::thread(attrs, boost::bind(&Sender::run, senderObj));
        thList.push_back(th);

    } else {
        usage();
        return 0;
    }

    for (auto it : thList) {
        it->join();
    }
    gettimeofday(&timeend, NULL);
    long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    double second = diff / 1000000.0;
    delete chunkerObj;
    delete keyClientObj;
    delete senderObj;
    delete recvDecodeObj;
    delete retrieverObj;
    delete powClientObj;
    delete encoderObj;
    cout << "System : total work time is " << second << " s" << endl;
    return 0;
}
