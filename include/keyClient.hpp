//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYCLIENT_HPP
#define GENERALDEDUPSYSTEM_KEYCLIENT_HPP

#include "Socket.hpp"
#include "_messageQueue.hpp"
#include "configure.hpp"
#include "seriazation.hpp"
#include "chunk.hpp"
#include "cache.hpp"
#include "powSession.hpp"
#include "kmServer.hpp"
#include "CryptoPrimitive.hpp"

#include <boost/compute/detail/lru_cache.hpp>

#define KEYMANGER_PUBLIC_KEY_FILE "key/serverpub.key"

class keyClient{
private:
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;
    int _keyBatchSizeMin,_keyBatchSizeMax;
    Socket _socket;
    bool _trustdKM;
    CryptoPrimitive _crypto;

public:
    keyClient();
    ~keyClient();
    void run();
    bool insertMQ(Chunk &newChunk);
    string keyExchange(Chunk champion);
};

#endif //GENERALDEDUPSYSTEM_KEYCLIENT_HPP
