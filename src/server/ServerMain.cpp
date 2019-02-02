//
// Created by a on 1/25/19.
//

#include "database.hpp"
#include "configure.hpp"
#include "_storage.hpp"
#include "_dataSR.hpp"
#include "_dedupCore.hpp"
#include "boost/thread.hpp"

Configure config("config.json");

database fp2ChunkDB;
database fileName2metaDB;

_StorageCore *storage;
_DataSR *dataSR;
_DedupCore *dedup;

int main(){
    vector<boost::thread*> thList;
    boost::thread *th;
    dataSR=new _DataSR();
    dedup=new _DedupCore();
    storage=new _StorageCore();

    int i,maxThread;

    //start dataSR
    th=new boost::thread(boost::bind(&_DataSR::workloadProgress,dataSR));
    thList.push_back(th);

    //start dedupCore
    maxThread=config.getMaxThreadLimits();
    for(i=0;i<maxThread;i++){
        th=new boost::thread(boost::bind(&_DedupCore::run,dedup));
        thList.push_back(th);
    }

    //start storageCore

    maxThread=config.getMaxThreadLimits();
    for(i=0;i<maxThread;i++){
        th=new boost::thread(boost::bind(&_StorageCore::run,storage));
        thList.push_back(th);
    }

    for(auto it:thList){
        it->join();
    }

    return 0;
}