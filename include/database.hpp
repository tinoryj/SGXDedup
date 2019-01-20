//
// Created by a on 12/23/18.
//

#ifndef GENERALDEDUPSYSTEM_DATABASE_HPP
#define GENERALDEDUPSYSTEM_DATABASE_HPP

#include "leveldb/db.h"

class database{
private:
    leveldb::DB *_levelDB= nullptr;
public:
    database();
    bool query(std::string key,std::string& value);
    bool insert(std::string key,std::string value);
};

#endif //GENERALDEDUPSYSTEM_DATABASE_HPP
