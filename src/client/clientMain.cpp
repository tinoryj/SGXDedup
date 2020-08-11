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
Encoder* encoderObj;
powClient* powClientObj;
Sender* senderObj;
RecvDecode* recvDecodeObj;
Retriever* retrieverObj;

struct timeval timestart;
struct timeval timeend;
struct timeval timestartBreakDown;
struct timeval timeendBreakDown;

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
    cerr << "Client exit with keyboard interrupt" << endl;
    if (chunkerObj != nullptr) {
        delete chunkerObj;
    }
    if (fingerprinterObj != nullptr) {
        delete fingerprinterObj;
    }
    if (keyClientObj != nullptr) {
        delete keyClientObj;
    }
    if (encoderObj != nullptr) {
        delete encoderObj;
    }
    if (powClientObj != nullptr) {
        delete powClientObj;
    }
    if (senderObj != nullptr) {
        delete senderObj;
    }
    if (recvDecodeObj != nullptr) {
        delete recvDecodeObj;
    }
    if (retrieverObj != nullptr) {
        delete retrieverObj;
    }
    exit(0);
}

void usage()
{
    cout << "[client --setup ] for setup system only, following input for system operation" << endl;
    cout << "[client -s filename] for send file" << endl;
    cout << "[client -r filename] for receive file" << endl;
    cout << "[client -k ThreadNumber keyNumber] for multi-thread key generate simluate" << endl;
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

    if (argv != 2 && argv != 3 && argv != 4) {
        usage();
        return 0;
    }
    if (strcmp("-r", argc[1]) == 0) {
        vector<boost::thread*> thList;
        boost::thread* th;
        boost::thread::attributes attrs;
        attrs.set_stack_size(200 * 1024 * 1024);
        gettimeofday(&timestart, NULL);
        string fileName(argc[2]);
        recvDecodeObj = new RecvDecode(fileName);
        retrieverObj = new Retriever(fileName, recvDecodeObj);
        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        cout << "System : Init download time is " << second << " s" << endl;

        gettimeofday(&timestart, NULL);
        // start recv data & decrypt thread
        th = new boost::thread(attrs, boost::bind(&RecvDecode::run, recvDecodeObj));
        thList.push_back(th);
        // start write file thread
        th = new boost::thread(attrs, boost::bind(&Retriever::run, retrieverObj));
        thList.push_back(th);

        for (auto it : thList) {
            it->join();
        }

        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;

        delete recvDecodeObj;
        delete retrieverObj;

        cout << "System : total work time is " << second << " s" << endl;
#if MULTI_CLIENT_UPLOAD_TEST == 1
        cout << "System : start work time is " << timestart.tv_sec << " s, " << timestart.tv_usec << " us" << endl;
        cout << "System : finish work time is " << timeend.tv_sec << " s, " << timeend.tv_usec << " us" << endl;
#endif
        cout << endl;
        return 0;

    } else if (strcmp("-k", argc[1]) == 0) {
        vector<boost::thread*> thList;
        boost::thread* th;
        boost::thread::attributes attrs;
        attrs.set_stack_size(200 * 1024 * 1024);
        gettimeofday(&timestart, NULL);
        int threadNumber = atoi(argc[2]);
        int keyGenNumber = atoi(argc[3]);
        if (threadNumber == 0) {
            threadNumber = 1;
        }
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartBreakDown, NULL);
#endif
        u_char sessionKey[KEY_SERVER_SESSION_KEY_SIZE];
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CFB || KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
        senderObj = new Sender();
        if (!senderObj->getKeyServerSK(sessionKey)) {
            cerr << "Client : get key server session key failed" << endl;
            delete senderObj;
            return 0;
        } else {
            delete senderObj;
        }
#endif
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendBreakDown, NULL);
        diff = 1000000 * (timeendBreakDown.tv_sec - timestartBreakDown.tv_sec) + timeendBreakDown.tv_usec - timestartBreakDown.tv_usec;
        second = diff / 1000000.0;
        cout << "System : download key exchange session key time is " << second << " s" << endl;
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
        cout << "System : Init key generate simulator time is " << second << " s" << endl;

        gettimeofday(&timestart, NULL);
        for (int i = 0; i < threadNumber; i++) {
            th = new boost::thread(attrs, boost::bind(&KeyClient::runKeyGenSimulator, keyClientObj, i));
            thList.push_back(th);
        }
        for (auto it : thList) {
            it->join();
        }
        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;

        delete keyClientObj;

        cout << "System : total work time is " << second << " s" << endl;
#if MULTI_CLIENT_UPLOAD_TEST == 1
        cout << "System : start work time is " << timestart.tv_sec << " s, " << timestart.tv_usec << " us" << endl;
        cout << "System : finish work time is " << timeend.tv_sec << " s, " << timeend.tv_usec << " us" << endl;
#endif
        cout << endl;
        return 0;

    } else if (strcmp("-s", argc[1]) == 0) {
        vector<boost::thread*> thList;
        boost::thread* th;
        boost::thread::attributes attrs;
        attrs.set_stack_size(200 * 1024 * 1024);

        gettimeofday(&timestart, NULL);

        senderObj = new Sender();
        powClientObj = new powClient(senderObj);
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timestartBreakDown, NULL);
#endif
        u_char sessionKey[KEY_SERVER_SESSION_KEY_SIZE];
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CFB || KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
        if (!senderObj->getKeyServerSK(sessionKey)) {
            cerr << "Client : get key server session key failed" << endl;
            delete senderObj;
            delete powClientObj;
            return 0;
        }
#endif
#if SYSTEM_BREAK_DOWN == 1
        gettimeofday(&timeendBreakDown, NULL);
        diff = 1000000 * (timeendBreakDown.tv_sec - timestartBreakDown.tv_sec) + timeendBreakDown.tv_usec - timestartBreakDown.tv_usec;
        second = diff / 1000000.0;
        cout << "System : download key exchange session key time is " << second << " s" << endl;
#endif
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
#if FINGERPRINTER_MODULE_ENABLE == 1
        fingerprinterObj = new Fingerprinter(keyClientObj);
        string inputFile(argc[2]);
        chunkerObj = new Chunker(inputFile, fingerprinterObj);
#else
        string inputFile(argc[2]);
        chunkerObj = new Chunker(inputFile, keyClientObj);
#endif
        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        cout << "System : Init upload time is " << second << " s" << endl;

#if MULTI_CLIENT_UPLOAD_TEST == 1
        cerr << "System : input sync number for multi client test" << endl;
        int inputNumberUsedForSyncInMultiClientTest = 0;
        cin >> inputNumberUsedForSyncInMultiClientTest;
#endif
        gettimeofday(&timestart, NULL);
        //start chunking thread
        th = new boost::thread(attrs, boost::bind(&Chunker::chunking, chunkerObj));
        thList.push_back(th);
#if FINGERPRINTER_MODULE_ENABLE == 1
        //start fingerprinting thread
        th = new boost::thread(attrs, boost::bind(&Fingerprinter::run, fingerprinterObj));
        thList.push_back(th);
#endif
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

        for (auto it : thList) {
            it->join();
        }
        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        delete senderObj;
        delete powClientObj;
#if ENCODER_MODULE_ENABLED == 1
        delete encoderObj;
#endif
        delete keyClientObj;
#if FINGERPRINTER_MODULE_ENABLE == 1
        delete fingerprinterObj;
#endif
        delete chunkerObj;

        cout << "System : upload total work time is " << second << " s" << endl;
#if MULTI_CLIENT_UPLOAD_TEST == 1
        cout << "System : start work time is " << timestart.tv_sec << " s, " << timestart.tv_usec << " us" << endl;
        cout << "System : finish work time is " << timeend.tv_sec << " s, " << timeend.tv_usec << " us" << endl;
#endif
        cout << endl;
        return 0;

    } else if (strcmp("--setup", argc[1]) == 0) {

        senderObj = new Sender();
        powClientObj = new powClient(senderObj);
        gettimeofday(&timeend, NULL);
        diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        second = diff / 1000000.0;
        cout << "System : Setup time is " << second << " s" << endl;
        delete senderObj;
        while (true) {
            cerr << "System : Waiting for operation request input" << endl
                 << "\tstore : upload a new file" << endl
                 << "\trestore : download an old file" << endl
                 << "\tkeyGen : start a key generate simulator" << endl
                 << "\texit : exit client application" << endl;
            string method;
            cin >> method;
            gettimeofday(&timestart, NULL);
            if (method.compare("exit") == 0) {
                delete powClientObj;
                delete senderObj;
                cerr << "System : client application exit" << endl;
                break;
            } else if (method.compare("store") == 0) {
                vector<boost::thread*> thList;
                boost::thread* th;
                boost::thread::attributes attrs;
                attrs.set_stack_size(200 * 1024 * 1024);
                cerr << "System : upload need input file path : " << endl;
                string filePath;
                cin >> filePath;
                senderObj = new Sender();
                powClientObj->resetSenderObj(senderObj);
                u_char sessionKey[KEY_SERVER_SESSION_KEY_SIZE];
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CFB || KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
                if (!senderObj->getKeyServerSK(sessionKey)) {
                    cerr << "Client : get key server session key failed, client exit" << endl;
                    delete senderObj;
                    delete powClientObj;
                    return 0;
                }
#endif
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
#if FINGERPRINTER_MODULE_ENABLE == 1
                fingerprinterObj = new Fingerprinter(keyClientObj);
                chunkerObj = new Chunker(filePath, fingerprinterObj);
#else
                chunkerObj = new Chunker(filePath, keyClientObj);
#endif
                gettimeofday(&timestart, NULL);
                //start chunking thread
                th = new boost::thread(attrs, boost::bind(&Chunker::chunking, chunkerObj));
                thList.push_back(th);
#if FINGERPRINTER_MODULE_ENABLE == 1
                //start fingerprinting thread
                th = new boost::thread(attrs, boost::bind(&Fingerprinter::run, fingerprinterObj));
                thList.push_back(th);
#endif
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

                for (auto it : thList) {
                    it->join();
                }
                gettimeofday(&timeend, NULL);
                diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
                second = diff / 1000000.0;
                cout << "System : total work time is " << second << " s" << endl;
#if MULTI_CLIENT_UPLOAD_TEST == 1
                cout << "System : start work time is " << timestart.tv_sec << " s, " << timestart.tv_usec << " us" << endl;
                cout << "System : finish work time is " << timeend.tv_sec << " s, " << timeend.tv_usec << " us" << endl;
#endif
                cout << endl;
                delete senderObj;
#if ENCODER_MODULE_ENABLED == 1
                delete encoderObj;
                delete keyClientObj;
#else
                delete keyClientObj;
#endif
#if FINGERPRINTER_MODULE_ENABLE == 1
                delete fingerprinterObj;
                delete chunkerObj;
#else
                delete chunkerObj;
#endif
            } else if (method.compare("restore") == 0) {
                vector<boost::thread*> thList;
                boost::thread* th;
                boost::thread::attributes attrs;
                attrs.set_stack_size(200 * 1024 * 1024);

                cerr << "System : download need input file path : " << endl;
                string filePath;
                cin >> filePath;
                recvDecodeObj = new RecvDecode(filePath);
                retrieverObj = new Retriever(filePath, recvDecodeObj);

                gettimeofday(&timestart, NULL);

                th = new boost::thread(attrs, boost::bind(&RecvDecode::run, recvDecodeObj));
                thList.push_back(th);
                th = new boost::thread(attrs, boost::bind(&Retriever::run, retrieverObj));
                thList.push_back(th);
                for (auto it : thList) {
                    it->join();
                }

                gettimeofday(&timeend, NULL);
                diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
                second = diff / 1000000.0;
                cout << "System : total work time is " << second << " s" << endl;
#if MULTI_CLIENT_UPLOAD_TEST == 1
                cout << "System : start work time is " << timestart.tv_sec << " s, " << timestart.tv_usec << " us" << endl;
                cout << "System : finish work time is " << timeend.tv_sec << " s, " << timeend.tv_usec << " us" << endl;
#endif
                cout << endl;
                delete recvDecodeObj;
                delete retrieverObj;

            } else if (method.compare("keyGen") == 0) {
                vector<boost::thread*> thList;
                boost::thread* th;
                boost::thread::attributes attrs;
                attrs.set_stack_size(200 * 1024 * 1024);
                cerr << "System : key generate simulator need input thread number & key number : " << endl;
                int threadNumber, keyGenNumber;
                cin >> threadNumber >> keyGenNumber;
                if (threadNumber == 0) {
                    threadNumber = 1;
                }
                u_char sessionKey[KEY_SERVER_SESSION_KEY_SIZE];
#if KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CFB || KEY_GEN_METHOD_TYPE == KEY_GEN_SGX_CTR
                senderObj = new Sender();
                powClientObj->resetSenderObj(senderObj);
                if (!senderObj->getKeyServerSK(sessionKey)) {
                    cerr << "Client : get key server session key failed, client exit" << endl;
                    delete senderObj;
                    delete powClientObj;
                    return 0;
                } else {
                    delete senderObj;
                }
#endif
#if SYSTEM_DEBUG_FLAG == 1
                cout << "System : recved key enclave session key = " << endl;
                PRINT_BYTE_ARRAY_CLIENT_MAIN(stderr, sessionKey, 32);
#endif
                cerr << "Key Generate Test : target thread number = " << threadNumber << ", target key number per thread = " << keyGenNumber << endl;
                keyClientObj = new KeyClient(sessionKey, keyGenNumber);

                gettimeofday(&timestart, NULL);
                for (int i = 0; i < threadNumber; i++) {
                    th = new boost::thread(attrs, boost::bind(&KeyClient::runKeyGenSimulator, keyClientObj, i));
                    thList.push_back(th);
                }
                for (auto it : thList) {
                    it->join();
                }
                gettimeofday(&timeend, NULL);
                diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
                second = diff / 1000000.0;
                cout << "System : total work time is " << second << " s" << endl;
#if MULTI_CLIENT_UPLOAD_TEST == 1
                cout << "System : start work time is " << timestart.tv_sec << " s, " << timestart.tv_usec << " us" << endl;
                cout << "System : finish work time is " << timeend.tv_sec << " s, " << timeend.tv_usec << " us" << endl;
#endif
                cout << endl;
                delete keyClientObj;
            }
        }
    } else {
        usage();
        return 0;
    }
}
