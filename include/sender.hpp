//
// Created by a on 1/12/19.
//

#ifndef GENERALDEDUPSYSTEM_SENDER_HPP
#define GENERALDEDUPSYSTEM_SENDER_HPP

#include "_sender.hpp"
#include "tmp.hpp"
#include "protocol.hpp"
#include "seriazation.hpp"
#include "configure.hpp"

class Sender:public _Sender {
private:
    boost::shared_mutex _sockMtx;
    Sock _socket;
public:
    Sender();

    ~Sender();

    //status define in protocol.hpp
    bool sendRecipe(Recipe &request, int &status);

    //status define in protocol.hpp
    bool sendChunkList(chunkList &request, int &status);

    //status define in protocol.hpp
    bool sendSignedHashList(powSignedHash &request,RequiredChunk &respond,int status);

    //send chunk when socket free
    void run();

    //general send data
    bool sendData(string &request, string &respond);
};

#endif //GENERALDEDUPSYSTEM_SENDER_HPP
