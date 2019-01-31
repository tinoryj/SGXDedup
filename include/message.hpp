//
// Created by a on 11/23/18.
//

#ifndef GENERALDEDUPSYSTEM_MESSAGE_HPP
#define GENERALDEDUPSYSTEM_MESSAGE_HPP
#include "ssl.hpp"
#include <string>
using namespace std;

struct message{
    int fd,epfd;
    char hash[128];
    char key[128];
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & fd;
        ar & epfd;
        ar & hash;
        ar & key;
    }
};


class epoll_message{
public:
    int _fd,_epfd;
    string _data;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _fd;
        ar & _epfd;
        ar & _data;
    }
};


#endif //GENERALDEDUPSYSTEM_MESSAGE_HPP
