//
// Created by a on 12/23/18.
//

#ifndef GENERALDEDUPSYSTEM_DATABASE_HPP
#define GENERALDEDUPSYSTEM_DATABASE_HPP

#include "leveldb/db.h"
#include "seriazation.hpp"
#include <bits/stdc++.h>
#include <boost/thread.hpp>
using namespace std;

struct keyValueForChunkHash {

    //key: string _chunkHash;
    //value: containerName, offset in container, chunk size;

    string _containerName;
    uint32_t _offset;
    uint32_t _length;

    void set(string containerName, uint32_t offset, uint32_t length)
    {
        this->_containerName = containerName;
        this->_offset = offset;
        this->_length = length;
    }
};

struct keyValueForFilename {

    //key: string _filename;
    //value: file recipe name, key recipe name, version;

    string _fileRecipeName;
    string _keyRecipeName;
    uint32_t _version;

    void set(string fileRecipe, string keyRecipe, uint32_t version)
    {
        this->_fileRecipeName = fileRecipe;
        this->_keyRecipeName = keyRecipe;
        this->_version = version;
    }
};

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
