//
// Created by a on 12/23/18.
//

#ifndef GENERALDEDUPSYSTEM_DATABASE_HPP
#define GENERALDEDUPSYSTEM_DATABASE_HPP

#include "leveldb/db.h"
#include "seriazation.hpp"
#include <boost/thread.hpp>
#include <string>
#include <fstream>
using namespace std;

struct keyValueForChunkHash{

    //key:
    //string _chunkHash;

    //value:
    string _containerName;
    uint32_t _offset;
    uint32_t _length;

    void set(string containerName,uint32_t offset,uint32_t length){
        this->_containerName=containerName;
        this->_offset=offset;
        this->_length=length;
    }

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar&_containerName;
        ar&_offset;
        ar&_length;
    }
};

struct keyValueForFilename{

    //key:
    //string _filename;

    //value:
    string _fileRecipeName;
    string _keyRecipeName;
    uint32_t _version;

    void set(string fileRecipe,string keyRecipe,uint32_t version) {
        this->_fileRecipeName=fileRecipe;
        this->_keyRecipeName=keyRecipe;
        this->_version=version;
    }

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar&_fileRecipeName;
        ar&_keyRecipeName;
        ar&_version;
    }
};

class database{
private:
    leveldb::DB *_levelDB= nullptr;
    boost::shared_mutex _mtx;
    std::string _dbName;
public:
    database();
    database(std::string dbName);
    ~database();
    void openDB(std::string dbName);
    bool query(std::string key,std::string& value);
    bool insert(std::string key,std::string value);
};

#endif //GENERALDEDUPSYSTEM_DATABASE_HPP
