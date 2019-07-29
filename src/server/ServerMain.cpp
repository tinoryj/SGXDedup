#include "../pow/include/powServer.hpp"
#include "boost/thread.hpp"
#include "cache.hpp"
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
ChunkCache* cache;

StorageCore* storage;
DataSR* SR;
DedupCore* dedup;
powServer* Pow;
//Timer* timerObj;

void CTRLC(int s)
{
    cerr << "server close" << endl;

    // if (storage != nullptr)
    //     delete storage;

    // if (SR != nullptr)
    //     delete SR;

    // if (dedup != nullptr)
    //     delete dedup;

    // if (Pow != nullptr)
    //     delete Pow;

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
    //timerObj = new Timer(cache);
    // SR = new DataSR();
    // dedup = new DedupCore();
    // storage = new StorageCore();
    // Pow = new powServer();

    // int i, maxThread;

    // //start DataSR
    // th = new boost::thread(boost::bind(&DataSR::run, SR));
    // thList.push_back(th);

    // //start pow
    // th = new boost::thread(boost::bind(&powServer::run, Pow));
    // thList.push_back(th);

    // //start DedupCore
    // maxThread = config.getDedupCoreThreadLimit();
    // for (i = 0; i < maxThread; i++) {
    //     th = new boost::thread(boost::bind(&DedupCore::run, dedup));
    //     thList.push_back(th);
    // }

    // //start StorageCore

    // maxThread = config.getStorageCoreThreadLimit();
    // for (i = 0; i < maxThread; i++) {
    //     th = new boost::thread(boost::bind(&StorageCore::run, storage));
    //     thList.push_back(th);
    // }

    for (auto it : thList) {
        it->join();
    }

    return 0;
}