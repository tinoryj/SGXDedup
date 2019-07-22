#ifndef GENERALDEDUPSYSTEM_RECIVER_HPP
#define GENERALDEDUPSYSTEM_RECIVER_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include "socket.hpp"
#include <boost/thread.hpp>

class receiver {
private:
    Socket socket_;
    void receiveChunk();

public:
    receiver();
    ~receiver();
    void run(std::string fileName);
    bool receiveData(string& data, int status);
};

#endif //GENERALDEDUPSYSTEM_RECIVER_HPP
