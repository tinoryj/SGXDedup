//
// Created by a on 2/3/19.
//

#ifndef GENERALDEDUPSYSTEM_DATASR_HPP
#define GENERALDEDUPSYSTEM_DATASR_HPP

#include "boost/thread.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"

class dataSR {
public:
    void run();
    bool receiveData();
    bool sendData();
};

#endif //GENERALDEDUPSYSTEM_DATASR_HPP
