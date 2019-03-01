//
// Created by a on 2/1/19.
//

#ifndef GENERALDEDUPSYSTEM_RECIPE_HPP
#define GENERALDEDUPSYSTEM_RECIPE_HPP

#include <string>
#include <vector>
#include <cstring>

#include "boost/thread.hpp"
using namespace std;

struct fileRecipe_t {
    string _fileName;        // =hash(origin name)
    uint32_t _fileSize;        // bytes
    //uint32_t _chunkCnt;
    string _createDate;

    fileRecipe_t() { _fileSize = 0; }

    struct body {
        uint32_t _chunkID;
        uint32_t _chunkSize;
        char _chunkHash[32];

        body() {};

        body(uint32_t id, uint32_t size, string hash) {
            _chunkID = id;
            _chunkSize = size;
            memset(_chunkHash, 0, sizeof(_chunkHash));
            memcpy(_chunkHash, &hash[0], hash.length());
        }

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & _chunkID;
            ar & _chunkSize;
            ar & _chunkHash;
        }
    };

    vector<body> _body;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _fileName;
        ar & _fileSize;
        ar & _createDate;
        ar & _body;
    }
};

struct keyRecipe_t{
    string _filename;	// =hash(origin name)
    uint32_t _fileSize;	// bytes
    string _createDate;
    keyRecipe_t(){_fileSize=0;}

    struct body{
        uint32_t _chunkID;
        char _chunkHash[32];
        string _chunkKey;

        body(){};

        body(uint32_t id,string hash,string key){
            _chunkID=id;
            memset(_chunkHash,0,sizeof(_chunkHash));
            memcpy(_chunkHash,&hash[0],hash.length());
            _chunkKey=key;
        }

        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & _chunkID;
            ar & _chunkHash;
            ar & _chunkKey;
        }
    };
    vector<body>_body;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _filename;
        ar & _fileSize;
        ar & _createDate;
        ar & _body;
    }
};

struct Recipe_t {
    fileRecipe_t _f;
    keyRecipe_t _k;
    string _kencrypted;
    int _chunkCnt;

    Recipe_t(){_chunkCnt=0;}

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _f;
        ar & _k;
        ar & _kencrypted;
    }
};

#endif //GENERALDEDUPSYSTEM_RECIPE_HPP
