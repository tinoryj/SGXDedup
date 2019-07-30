#include "../pow/include/powServer.hpp"
#include "boost/thread.hpp"
#include "cache.hpp"
#include "configure.hpp"
#include "dataSR.hpp"
#include "database.hpp"
#include "dedupCore.hpp"
#include "messageQueue.hpp"
#include "storageCore.hpp"
#include "timer.hpp"
#include <signal.h>
Configure config("config.json");

Database fp2ChunkDB;
Database fileName2metaDB;
ChunkCache* cache;

StorageCore* storageObj;
DataSR* dataSRObj;
Timer* timerObj;
DedupCore* dedupCoreObj;
powServer* powServerObj;
//Timer* timerObj;

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
    fileName2metaDB.openDB(config.getFn2MetaDBame());

    vector<boost::thread*> thList;
    boost::thread* th;
    cache = new ChunkCache();
    timerObj = new Timer();
    dataSRObj = new DataSR();
    dedupCoreObj = new DedupCore(dataSRObj, timerObj);
    storageObj = new StorageCore(dataSRObj, timerObj);
    powServerObj = new powServer(dataSRObj);

    boost::thread::attributes attrs;
    //cerr << attrs.get_stack_size() << endl;
    attrs.set_stack_size(100 * 1024 * 1024);
    //cerr << attrs.get_stack_size() << endl;

    //start DataSR
    th = new boost::thread(attrs, boost::bind(&DataSR::run, dataSRObj));
    thList.push_back(th);
    th = new boost::thread(attrs, boost::bind(&DataSR::extractMQ, dataSRObj));
    thList.push_back(th);

    //start pow
    th = new boost::thread(attrs, boost::bind(&powServer::run, powServerObj));
    thList.push_back(th);

    //start DedupCore
    for (int i = 0; i < config.getDedupCoreThreadLimit(); i++) {
        th = new boost::thread(attrs, boost::bind(&DedupCore::run, dedupCoreObj));
        thList.push_back(th);
    }

    //start StorageCore

    for (int i = 0; i < config.getStorageCoreThreadLimit(); i++) {
        th = new boost::thread(attrs, boost::bind(&StorageCore::storageThreadForDataSR, storageObj));
        thList.push_back(th);
        th = new boost::thread(attrs, boost::bind(&StorageCore::storageThreadForTimer, storageObj));
        thList.push_back(th);
    }

    for (auto it : thList) {
        it->join();
    }

    return 0;
}