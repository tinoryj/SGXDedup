//
// Created by a on 11/23/18.
//

#ifndef GENERALDEDUPSYSTEM_MESSAGE_HPP
#define GENERALDEDUPSYSTEM_MESSAGE_HPP
#include "ssl.hpp"
#include <string>
using namespace std;

struct networkStruct {
    int _type;
    int _cid;
    string _data;

    networkStruct(int msgType, int clientID) : _type(msgType), _cid(clientID) {};

    networkStruct() {};

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _type;
        ar & _cid;
        ar & _data;
    }
};


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
    int _fd,_epfd,_type,_cid;
    string _data;

    void setNetStruct(networkStruct &n){
        this->_type=n._type;
        this->_cid=n._cid;
        this->_data=n._data;
    }

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _fd;
        ar & _epfd;
        ar & _data;
        ar & _type;
        ar & _cid;
    }
};


#endif //GENERALDEDUPSYSTEM_MESSAGE_HPP
