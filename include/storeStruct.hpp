//
// Created by a on 1/23/19.
//

#ifndef GENERALDEDUPSYSTEM_STORESTRUCT_HPP
#define GENERALDEDUPSYSTEM_STORESTRUCT_HPP

#include <string>
#include "seriazation.hpp"
/*

struct _FileRecipe{
    std::string _fileName;
    uint32_t _fileSize;
    //uint32_t _chunkCnt;
    std::string _createData;
    struct body{
        uint32_t _chunkID;
        uint32_t _chunkSize;
        char _chunkHash[128];
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar&_chunkID;
            ar&_chunkSize;
            ar&_chunkHash;
        }
    };
    vector<body>_body;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar&_fileName;
        ar&_fileSize;
        //ar&_chunkCnt;
        ar&_createData;
        ar&_body;
    }
};

struct _KeyRecipe{
    std::string _filename;
    uint32_t _fileSize;
    std::string _createData;
    struct body{
        uint32_t _chunkID;
        uint32_t _chunkHash[128];
        std::string _chunkKey;
        template<class Archive>
        void serialize(Archive &ar, const unsigned int version) {
            ar&_chunkID;
            ar&_chunkHash;
            ar&_chunkKey;
        }
    };
    vector<body>_body;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar&_filename;
        ar&_fileSize;
        ar&_createData;
        ar&_body;
    }
};

*/
struct _Container{
    uint32_t _used;
    char _body[4<<20];  //4 M
    _Container();
    ~_Container();
};

#endif //GENERALDEDUPSYSTEM_STORESTRUCT_HPP
