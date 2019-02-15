//
// Created by a on 1/12/19.
//

#ifndef GENERALDEDUPSYSTEM_SENDER_HPP
#define GENERALDEDUPSYSTEM_SENDER_HPP

#include "_sender.hpp"
#include "protocol.hpp"
#include "seriazation.hpp"
#include "configure.hpp"
#include "CryptoPrimitive.hpp"

#include <sgx_uae_service.h>
#include <sgx_ukey_exchange.h>

class Sender:public _Sender {
private:
    boost::shared_mutex _sockMtx;
    Socket _socket;
public:
    Sender();

    ~Sender();

    //status define in protocol.hpp
    bool sendRecipe(Recipe_t &request, int &status);

    //status define in protocol.hpp
    bool sendChunkList(chunkList &request, int &status);

    //for pow
    bool sendSGXmsg01(uint32_t &msg0,sgx_ra_msg1_t &msg1,sgx_ra_msg2_t *msg2,int &status);
    bool sendSGXmsg3(sgx_ra_msg3_t &msg3,uint32_t sz,ra_msg4_t *msg4,int &status);
    bool sendEnclaveSignedHash(powSignedHash &request,RequiredChunk &respond,int status);

    //send chunk when socket free
    void run();

    //general send data
    bool sendData(string &request, string &respond);
};

#endif //GENERALDEDUPSYSTEM_SENDER_HPP
