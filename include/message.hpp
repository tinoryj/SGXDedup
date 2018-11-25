//
// Created by a on 11/23/18.
//

#ifndef GENERALDEDUPSYSTEM_MESSAGE_HPP
#define GENERALDEDUPSYSTEM_MESSAGE_HPP
#include "ssl.hpp"

struct message{
    //std::pair<int,SSL*> con;
    connection con;
    int epfd;
    char hash[16];
    char key[16];
};

#endif //GENERALDEDUPSYSTEM_MESSAGE_HPP
