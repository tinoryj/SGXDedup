#ifndef GENERALDEDUPSYSTEM_DATABASE_HPP
#define GENERALDEDUPSYSTEM_DATABASE_HPP

#include "dataStructure.hpp"
#include "leveldb/db.h"
#include "seriazation.hpp"
#include <bits/stdc++.h>
#include <boost/thread.hpp>
using namespace std;

class database {
private:
    leveldb::DB* levelDBObj_ = nullptr;
    std::mutex mutexDataBase_;
    std::string dbName_;

public:
    database(){};
    database(std::string dbName);
    ~database();
    bool openDB(std::string dbName);
    bool query(std::string key, std::string& value);
    bool insert(std::string key, std::string value);
};

#endif //GENERALDEDUPSYSTEM_DATABASE_HPP
