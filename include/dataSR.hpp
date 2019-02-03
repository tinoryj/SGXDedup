//
// Created by a on 2/3/19.
//

#ifndef GENERALDEDUPSYSTEM_DATASR_HPP
#define GENERALDEDUPSYSTEM_DATASR_HPP

#include "_dataSR.hpp"
class dataSR:public _DataSR{
public:
    void run();
    bool receiveData();
    bool sendData();
};

#endif //GENERALDEDUPSYSTEM_DATASR_HPP
