#include "../pow/include/powClient.hpp"
#include "chunker.hpp"
#include "configure.hpp"
#include "encoder.hpp"
#include "fingerprinter.hpp"
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
Fingerprinter* fingerprinterObj;
KeyClient* keyClientObj;
#if ENCODER_MODULE_ENABLED == 1
Encoder* encoderObj;
#endif
powClient* powClientObj;
Sender* senderObj;
RecvDecode* recvDecodeObj;
Retriever* retrieverObj;

struct timeval timestart;
struct timeval timeend;

void PRINT_BYTE_ARRAY_CLIENT_MAIN(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

void CTRLC(int s)
{
    cerr << "Client close" << endl;
    delete chunkerObj;
    delete fingerprinterObj;
    delete keyClientObj;
#if ENCODER_MODULE_ENABLED == 1
    delete encoderObj;
#endif
    delete powClientObj;
    delete senderObj;
    delete recvDecodeObj;
    delete retrieverObj;
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

    long diff;
    double second;
    gettimeofday(&timestart, NULL);
    vector<boost::thread*> thList;
    boost::thread* th;
    if (argv != 3 && argv != 4) {
        usage();
        return 0;
    }
    boost::thread::attributes attrs;
    attrs.set_stack_size(200 * 1024 * 1024);

    if (strcmp("-r", argc[1]) == 0) {
        string fileName(argc[2]);

        recvDecodeObj = new RecvDecode(fileName);
        retrieverObj = new Retriever(fileName, recvDecodeObj);

        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        cout << "System : Init time is " << second << " s" << endl;

        gettimeofday(&timestart, NULL);

        th = new boost::thread(attrs, boost::bind(&RecvDecode::run, recvDecodeObj));
        thList.push_back(th);

        th = new boost::thread(attrs, boost::bind(&Retriever::run, retrieverObj));
        thList.push_back(th);

    } else if (strcmp("-k", argc[1]) == 0) {
        int threadNumber = atoi(argc[2]);
        if (threadNumber == 0) {
            threadNumber = 1;
        }
        int keyGenNumber = atoi(argc[3]);
        u_char sessionKey[KEY_SERVER_SESSION_KEY_SIZE];
#if KEY_GEN_SGX_CFB == 1 || KEY_GEN_SGX_CTR == 1
        senderObj = new Sender();
        if (!senderObj->getKeyServerSK(sessionKey)) {
            cerr << "Client : get key server session key failed" << endl;
            delete senderObj;
            return 0;
        }
#endif
#if SYSTEM_DEBUG_FLAG == 1
        cout << "System : recved key enclave session key = " << endl;
        PRINT_BYTE_ARRAY_CLIENT_MAIN(stderr, sessionKey, 32);
#endif
        cout << "Key Generate Test : target thread number = " << threadNumber << ", target key number per thread = " << keyGenNumber << endl;
        keyClientObj = new KeyClient(sessionKey, keyGenNumber);

        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        cout << "System : Init time is " << second << " s" << endl;

        gettimeofday(&timestart, NULL);
        for (int i = 0; i < threadNumber; i++) {
            th = new boost::thread(attrs, boost::bind(&KeyClient::runKeyGenSimulator, keyClientObj, i));
            thList.push_back(th);
        }
    } else if (strcmp("-s", argc[1]) == 0) {

        senderObj = new Sender();
        powClientObj = new powClient(senderObj);

        u_char sessionKey[KEY_SERVER_SESSION_KEY_SIZE];
        if (!senderObj->getKeyServerSK(sessionKey)) {
            cerr << "Client : get key server session key failed" << endl;
            delete senderObj;
            delete powClientObj;
            return 0;
        }
#if SYSTEM_DEBUG_FLAG == 1
        cout << "System : recved key enclave session key = " << endl;
        PRINT_BYTE_ARRAY_CLIENT_MAIN(stderr, sessionKey, 32);
#endif
#if ENCODER_MODULE_ENABLED == 1
        encoderObj = new Encoder(powClientObj);
        keyClientObj = new KeyClient(encoderObj, sessionKey);
#else
        keyClientObj = new KeyClient(powClientObj, sessionKey);
#endif
        fingerprinterObj = new Fingerprinter(keyClientObj);
        string inputFile(argc[2]);
        chunkerObj = new Chunker(inputFile, fingerprinterObj);

        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        cout << "System : Init time is " << second << " s" << endl;

        gettimeofday(&timestart, NULL);
        //start chunking thread
        th = new boost::thread(attrs, boost::bind(&Chunker::chunking, chunkerObj));
        thList.push_back(th);

        //start fingerprinting thread
        th = new boost::thread(attrs, boost::bind(&Fingerprinter::run, fingerprinterObj));
        thList.push_back(th);

        //start key client thread
        th = new boost::thread(attrs, boost::bind(&KeyClient::run, keyClientObj));
        thList.push_back(th);
#if ENCODER_MODULE_ENABLED == 1
        //start encoder thread
        th = new boost::thread(attrs, boost::bind(&Encoder::run, encoderObj));
        thList.push_back(th);
#endif
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
    diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
    second = diff / 1000000.0;

    delete chunkerObj;
    delete fingerprinterObj;
    delete keyClientObj;
#if ENCODER_MODULE_ENABLED == 1
    delete encoderObj;
#endif
    delete powClientObj;
    delete senderObj;
    delete recvDecodeObj;
    delete retrieverObj;

    cout << "System : total work time is " << second << " s" << endl;
    return 0;
}
