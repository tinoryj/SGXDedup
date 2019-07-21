//
// Created by a on 1/25/19.
//

#include "../pow/include/PowServer.hpp"
#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataSR.hpp"
#include "database.hpp"
#include "dedupCore.hpp"
#include "messageQueue.hpp"
#include "storageCore.hpp"
#include <signal.h>
Configure config("config.json");

database fp2ChunkDB;
database fileName2metaDB;

storageCore* storage;
dataSR* SR;
dedupCore* dedup;
powServer* Pow;

void CTRLC(int s)
{
    cerr << "server close" << endl;

    if (storage != nullptr)
        delete storage;

    if (SR != nullptr)
        delete SR;

    if (dedup != nullptr)
        delete dedup;

    if (Pow != nullptr)
        delete Pow;

    exit(0);
}

int main()
{

    initMQForServer();

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
    SR = new dataSR();
    dedup = new dedupCore();
    storage = new storageCore();
    Pow = new powServer();

    int i, maxThread;

    //start dataSR
    th = new boost::thread(boost::bind(&dataSR::run, SR));
    thList.push_back(th);

    //start pow
    th = new boost::thread(boost::bind(&powServer::run, Pow));
    thList.push_back(th);

    //start dedupCore
    maxThread = config.getDedupCoreThreadLimit();
    for (i = 0; i < maxThread; i++) {
        th = new boost::thread(boost::bind(&dedupCore::run, dedup));
        thList.push_back(th);
    }

    //start storageCore

    maxThread = config.getStorageCoreThreadLimit();
    for (i = 0; i < maxThread; i++) {
        th = new boost::thread(boost::bind(&storageCore::run, storage));
        thList.push_back(th);
    }

    for (auto it : thList) {
        it->join();
    }

    return 0;
}