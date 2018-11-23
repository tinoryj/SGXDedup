//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_KEYSERVER_HPP
#define GENERALDEDUPSYSTEM_KEYSERVER_HPP

#include "_keyManager.hpp"
#include "chunk.hpp"
#include "ssl.hpp"
#include "configure.hpp"
#include "cache.hpp"
#include "_messageQueue.hpp"
#include "message.hpp"

#include <string>

#define SERVERSIDE 0
#define CLIENTSIDE 1
#define KEYMANGER_PRIVATE_KEY "./key/private.pem"



class keyServer:public _keyManager{
private:
    ssl* _keySecurityChannel;
    RSA* _rsa;
    BIO* _key;
    BN_CTX* _bnCTX;

public:
    boost::compute::detail::lru_cache<string,string>* kCache;
    keyServer();
    ~keyServer();
    void run();
    void threadHandle();
    bool keyGen(std::string hash,std::string& key);
};

#endif //GENERALDEDUPSYSTEM_KEYSERVER_HPP
