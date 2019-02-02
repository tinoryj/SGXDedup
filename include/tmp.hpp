//
// Created by a on 1/27/19.
//
#include "seriazation.hpp"

#ifndef GENERALDEDUPSYSTEM_TMP_HPP
#define GENERALDEDUPSYSTEM_TMP_HPP



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
