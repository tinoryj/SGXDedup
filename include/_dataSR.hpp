#include "_messageQueue.hpp"
#include "boost/thread.hpp"
#include "boost/bind.hpp"
#include "sys/epoll.h"
#include "Socket.hpp"
#include "protocol.hpp"
#include <string>
using namespace std;

#define MESSAGE2DUPCORE 0
#define MESSAGE2STORAGE 1
#define MESSAGE2RASERVER 2

class _DataSR {
private:
    _messageQueue _inputMQ;
    _messageQueue _mq2RAServer;
    _messageQueue _mq2DedupCore;
    _messageQueue _mq2StorageCore;
    //_messageQueue _outputMQ[3];
    Socket _socket;
    // any additional info
public:
    _DataSR();
    ~_DataSR();
    virtual bool receiveData() = 0;
    virtual bool sendData() = 0;
    bool workloadProgress(); // main function for epoll S/R and threadPool schedule (insertMQ & extractMQ threads).
    bool insertMQ(int queueSwitch,epoll_message *msg);
    bool extractMQ();
    //MessageQueue getInputMQ();
    //MessageQueue getOutputMQ();
    // any additional functions
};