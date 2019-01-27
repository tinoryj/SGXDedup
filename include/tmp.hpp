//
// Created by a on 1/27/19.
//
#include "seriazation.hpp"

#ifndef GENERALDEDUPSYSTEM_TMP_HPP
#define GENERALDEDUPSYSTEM_TMP_HPP

struct networkStruct{
    int _type;
    int _cid;
    string _data;

    networkStruct(int msgType,int clientID):_type(msgType),_cid(clientID){};
    networkSttuct(){};

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _type;
        ar & _cid;
        ar & _data;
    }
};

struct epollMessageStruct{
    int _fd;
    int _epfd;
    string _data;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _fd;
        ar & _epfd;
        ar&_data;
    }

};

struct chunkList{
    vector<string>_FP;
    vector<string>_chunks;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _FP;
        ar & _chunks;
    }
};

struct fileRecipe{
    string _fileName;		// =hash(origin name)
    uint32_t _fileSize;		// bytes
    //uint32_t _chunkCnt;
    string _createData;
    struct body{
        uint32_t _chunkID;
        uint32_t _chunkSize;
        char _chunkHash[128];
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & _chunkID;
            ar & _chunkSize;
            ar&_chunkHash;
        }
    };
    vector<body>_body;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _fileSize;
        ar & _fileSize;
        ar&_createData;
        ar& _body;
    }
};

struct keyRecipe{
    string _filename;	// =hash(origin name)
    uint32_t _fileSize;	// bytes
    string _createData;
    struct body{
        uint32_t _chunkID;
        char _chunkHash[128];
        string _chunkKey;
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar & _chunkID;
            ar & _chunkHash;
            ar&_chunkKey;
        }
    };
    vector<body>_body;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _filename;
        ar & _fileSize;
        ar&_createData;
        ar&_body;
    }
};

struct Recipe{
    fileRecipe f;
    keyRecipe k;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _f;
        ar & _k;
    }
};

typedef vector<int> RequiredChunk;

struct powSignedHash{
    char signature[128];
    vector<string>hash;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & signature;
        ar & hash;
    }
};

#endif //GENERALDEDUPSYSTEM_TMP_HPP
