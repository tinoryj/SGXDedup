#include "_keyManager.hpp"

_KeyManager::_KeyManager(){}
_KeyManager::~_KeyManager(){}
bool _KeyManager::workloadProgress();
bool _KeyManager::insertQue(){}
bool _KeyManager::extractQue(){}
std::deque<Chunk> _KeyManager::getReceiveQue(){}
std::deque<Chunk> _KeyManager::getSendQue(){}
