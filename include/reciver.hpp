//
// Created by a on 1/21/19.
//

#ifndef GENERALDEDUPSYSTEM_RECIVER_HPP
#define GENERALDEDUPSYSTEM_RECIVER_HPP

#include "socket.hpp"
#include <boost/thread.hpp>
class receiver {
private:
    Socket _socket;
    void receiveChunk();

public:
    receiver();
    ~receiver();
    void run(std::string fileName);
    bool receiveData(string& data, int status);
};

#endif //GENERALDEDUPSYSTEM_RECIVER_HPP
