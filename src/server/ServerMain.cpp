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


int main(){

    return 0;
}