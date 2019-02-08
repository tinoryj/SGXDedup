//
// Created by a on 1/25/19.
//

#include "database.hpp"
#include "configure.hpp"
#include "storageCore.hpp"
#include "dataSR.hpp"
#include "dedupCore.hpp"
#include "boost/thread.hpp"

Configure config("config.json");

database fp2ChunkDB;
database fileName2metaDB;

storageCore *storage;
dataSR *SR;
dedupCore *dedup;

int main(){
    vector<boost::thread*> thList;
    boost::thread *th;
    SR=new dataSR();
    dedup=new dedupCore();
    storage=new storageCore();

    int i,maxThread;

    //start dataSR
    th=new boost::thread(boost::bind(&dataSR::run,SR));
    thList.push_back(th);

    //start dedupCore
    maxThread=config.getDedupCoreThreadLimit();
    for(i=0;i<maxThread;i++){
        th=new boost::thread(boost::bind(&dedupCore::run,dedup));
        thList.push_back(th);
    }

    //start storageCore

    maxThread=config.getStorageCoreThreadLimit();
    for(i=0;i<maxThread;i++){
        th=new boost::thread(boost::bind(&storageCore::run,storage));
        thList.push_back(th);
    }

    for(auto it:thList){
        it->join();
    }

    return 0;
}