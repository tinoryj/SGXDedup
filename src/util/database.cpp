//
// Created by a on 12/23/18.
//

#include "database.hpp"
using namespace std;
using namespace leveldb;

bool database::query(std::string key, std::string &value) {
    Status queryStatus=this->_levelDB->Get(ReadOptions(),key,&value);
    return queryStatus.ok();
}

bool database::insert(std::string key, std::string value) {
    Status insertStatus=this->_levelDB->Put(WriteOptions(),key,value);
    return insertStatus.ok();
}

void database::openDB(std::string dbName) {
    fstream dbLock;
    dbLock.open("."+dbName+".lock",std::fstream::in);
    if(dbLock.is_open()){
        dbLock.close();
        std::cerr<<"Database lock\n";
        return ;
    }

    dbLock.open("."+dbName+".lock",std::fstream::out);
    dbLock<<"lock";
    dbLock.close();

    _dbName=dbName;

    leveldb::Options options;
    options.create_if_missing=true;
    leveldb::Status status=leveldb::DB::Open(options,dbName,&this->_levelDB);
    assert(status.ok());
}

database::database() {}

database::database(std::string dbName) {
    this->openDB(dbName);
}

database::~database() {
    string name="."+_dbName+".lock";
    remove(name.c_str());
}