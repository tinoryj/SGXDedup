//
// Created by a on 1/21/19.
//

#ifndef GENERALDEDUPSYSTEM_RECIVER_HPP
#define GENERALDEDUPSYSTEM_RECIVER_HPP

#include "_receiver.hpp"
#include <boost/thread.hpp>
#include "seriazation.hpp"

class receiver:public _Receiver{
private:
    Sock _socket;
    void receiveChunk();

public:
    receiver();
    ~receiver();
    void run(std::string fileName);
};

/*

class receiver:public _Receiver{
private:
    _messageQueue _outputMQ;
    Sock _socket;
    boost::shared_lock _socketMtx;
    bool readyForChunkDownload;

public:
    receiver();
    ~receiver();
    bool getFileRecipe(fileRecipe &respond,int &status);
    bool getKeyRecipe(string &respond,int &status);
    bool getChunk(Chunk &respond,int &status);
    bool receiveData(string &respond,int &status);
};
*/


#endif //GENERALDEDUPSYSTEM_RECIVER_HPP
