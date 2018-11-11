#include "_keyManager.hpp"

_keyManager::_keyManager(){}
_keyManager::~_keyManager(){}

bool _keyManager::workloadProgress(std::string hash,std::string &key){
    /*
    lock cache
    look for cache
    frepp cache
    if(find) return
    */
    return keyGen(hash,key);
}
bool _keyManager::insertQue(){

    return true;
}

bool _keyManager::extractQue(){

    return true;
}