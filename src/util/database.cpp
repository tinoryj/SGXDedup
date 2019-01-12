//
// Created by a on 12/23/18.
//

#include "database.hpp"
using namespace std;
using namespace leveldb;

bool database::query(std::string key, std::string &value) {
    Status queryStatus=this->_levelDB->Get(ReadOptions(),key,&value);
    return Status.ok();
}

bool database::insert(std::string key, std::string value) {
    Status insertStatus=this->_levelDB->Put(WriteOptions(),key.value);
    return Status.ok();
}