#include "../pow/include/powServer.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataSR.hpp"
#include "database.hpp"
#include "dedupCore.hpp"
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

    boost::thread* th;
    dedupCoreObj = new DedupCore();
    storageObj = new StorageCore();
    powServerObj = new powServer();
    dataSRObj = new DataSR(storageObj, dedupCoreObj, powServerObj);

    boost::thread::attributes attrs;
    //cerr << attrs.get_stack_size() << endl;
    attrs.set_stack_size(200 * 1024 * 1024);
    //cerr << attrs.get_stack_size() << endl;
    Socket socket(SERVER_TCP, "", config.getStorageServerPort());
    while (true) {
        Socket tmpSocket = socket.Listen();
        th = new boost::thread(attrs, boost::bind(&DataSR::run, dataSRObj, tmpSocket));
        thList.push_back(th);
    }

    return 0;
}