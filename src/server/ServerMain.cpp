#include "../pow/include/powServer.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataSR.hpp"
#include "database.hpp"
#include "dedupCore.hpp"
#include "kmServer.hpp"
#include "messageQueue.hpp"
#include "storageCore.hpp"
#include <signal.h>
Configure config("config.json");

Database fp2ChunkDB;
Database fileName2metaDB;

DataSR* dataSRObj;
StorageCore* storageObj;
DedupCore* dedupCoreObj;
powServer* powServerObj;

vector<boost::thread*> thList;

void CTRLC(int s)
{
    cerr << "server close" << endl;

    if (storageObj != nullptr)
        delete storageObj;

    if (dataSRObj != nullptr)
        delete dataSRObj;

    if (dedupCoreObj != nullptr)
        delete dedupCoreObj;

    if (powServerObj != nullptr)
        delete powServerObj;
    for (auto it : thList) {
        it->join();
    }
    exit(0);
}

int main()
{

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    sa.sa_handler = CTRLC;
    sigaction(SIGKILL, &sa, 0);
    sigaction(SIGINT, &sa, 0);

    fp2ChunkDB.openDB(config.getFp2ChunkDBName());
    fileName2metaDB.openDB(config.getFp2MetaDBame());

    Socket socket_;
    socket_.init(CLIENT_TCP, config.getKeyServerIP(), config.getKeyServerPort());
    kmServer server(socket_);
    powSession* session = server.authkm();
    u_char keyExchangeKey_[16];
    if (session != nullptr) {
        memcpy(keyExchangeKey_, session->sk, 16);
        cerr << "KeyClient : keyServer enclave trusted" << endl;
        delete session;
    } else {
        delete session;
        cerr << "KeyClient : keyServer enclave not trusted, storage server exit now" << endl;
        return 0;
    }

    dedupCoreObj = new DedupCore();
    storageObj = new StorageCore();
    powServerObj = new powServer();
    dataSRObj = new DataSR(storageObj, dedupCoreObj, powServerObj, keyExchangeKey_);

    boost::thread* th;
    boost::thread::attributes attrs;
    //cerr << attrs.get_stack_size() << endl;
    attrs.set_stack_size(200 * 1024 * 1024);
    //cerr << attrs.get_stack_size() << endl;
    Socket socketData(SERVER_TCP, "", config.getStorageServerPort());
    Socket socketPow(SERVER_TCP, "", config.getPOWServerPort());
    while (true) {
        Socket tmpSocket = socketData.Listen();
        th = new boost::thread(attrs, boost::bind(&DataSR::run, dataSRObj, tmpSocket));
        thList.push_back(th);
        Socket tmpSocketPow = socketPow.Listen();
        th = new boost::thread(attrs, boost::bind(&DataSR::runPow, dataSRObj, tmpSocketPow));
        thList.push_back(th);
    }

    return 0;
}