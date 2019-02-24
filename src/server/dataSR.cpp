//
// Created by a on 2/3/19.
//

#include <dataSR.hpp>

void dataSR::run(){
    boost::thread th(boost::bind(&_DataSR::extractMQ,this));
    this->workloadProgress();
    th.detach();
}

bool dataSR::receiveData() {}
bool dataSR::sendData() {}