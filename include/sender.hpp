//
// Created by a on 1/12/19.
//

#ifndef GENERALDEDUPSYSTEM_SENDER_HPP
#define GENERALDEDUPSYSTEM_SENDER_HPP

#include "_sender.hpp"

class Sender:public _Sender{
public:
    void run();
    Sender();
    ~Sender();

private:
    bool sendData(Chunk& tmpChunk);
    Sock Socket;
};



#endif //GENERALDEDUPSYSTEM_SENDER_HPP
