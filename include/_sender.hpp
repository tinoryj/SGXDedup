#ifndef GENERALDEDUPSYSTEM__SENDER_HPP
#define GENERALDEDUPSYSTEM__SENDER_HPP

#include "chunk.hpp"
#include "Socket.hpp"
#include "_messageQueue.hpp"
#include "configure.hpp"
#include "seriazation.hpp"


class _Sender {
private:
    _messageQueue _inputMQ;
    _messageQueue getInputMQ();
    // any additional info
public:
    _Sender();
    ~_Sender();
    bool extractMQ(Chunk &tmpChunk);
    //Implemented in a derived class and implements different types of transmissions by overloading the function
    virtual bool sendData(string &request,string &respond) = 0;
    // any additional functions
};


#endif