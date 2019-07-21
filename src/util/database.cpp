//
// Created by a on 12/23/18.
//

#include "database.hpp"

bool database::query(std::string key, std::string& value)
{
    //std::lock_guard<std::mutex> locker(this->mutexDataBase_);
    leveldb::Status queryStatus = this->levelDBObj_->Get(leveldb::ReadOptions(), key, &value);
    return queryStatus.ok();
}

bool database::insert(std::string key, std::string value)
{
    std::lock_guard<std::mutex> locker(this->mutexDataBase_);
    leveldb::Status insertStatus = this->levelDBObj_->Put(leveldb::WriteOptions(), key, value);
    return insertStatus.ok();
}

bool database::openDB(std::string dbName)
{
    fstream dbLock;
    dbLock.open("." + dbName + ".lock", std::fstream::in);
    if (dbLock.is_open()) {
        dbLock.close();
        std::cerr << "Database lock" << endl;
        return false;
    }
    dbName_ = dbName;

    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbName, &this->levelDBObj_);
    assert(status.ok());
    if (status.ok()) {
        return true;
    }
}

database::database(std::string dbName)
{
    this->openDB(dbName);
}

database::~database()
{
    std::string name = "." + dbName_ + ".lock";
    remove(name.c_str());
}