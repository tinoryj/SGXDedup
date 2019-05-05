//
// Created by a on 11/17/18.
//

#include "_keyManager.hpp"

_keyManager::_keyManager(){
//    receiveQue.createQueue("reveiver to workload",WRITE_MESSAGE);
//    sendQue.createQueue("workload to rereceive",WRITE_MESSAGE);
}
_keyManager::~_keyManager(){}

bool _keyManager::workloadProgress(std::string hash,std::string &key){
    return true;
}
bool _keyManager::insertQue(){

    return true;
}

bool _keyManager::extractQue(){

    return true;
}