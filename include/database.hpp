//
// Created by a on 12/23/18.
//

#ifndef GENERALDEDUPSYSTEM_DATABASE_HPP
#define GENERALDEDUPSYSTEM_DATABASE_HPP

#include "leveldb/db.h"
#include "seriazation.hpp"
#include <boost/thread.hpp>
#include <string>
using namespace std;

struct keyValueForChunkHash{

    //key:
    //string _chunkHash;

    //value:
    string _containerName;
    uint32_t _offset;
    uint32_t _length;

    void set(string containerName,uint32_t offset,uint32_T length){
        this->_chunkHash=key;
        this->_containerName=value.substr(0,value.length()-8);
        memcpy(&this->_offset,&value[value.length-8],sizeof(uint32_t));
        memcpy(&this->_length,&value[value.length-4],sizeof(uint32_t));
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
        this->_chunkHash=chunkHash;
        this->_containerName=containerName;
        this->_offset=offset;
        this->_length=length;
    }

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar&_fileRecipeName;
        ar&_keyRecipeName;
        ar&_version;
    }
};

void keyValueForFilename::set(string fileRecipeName, string keyRecipeName, uint32_t version) {
    this->_filename=filename;
    this->_fileRecipeName=fileRecipeName;
    this->_keyRecipeName=keyRecipeName;
    this->_version=version;
}


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
