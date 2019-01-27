#include "_messageQueue.hpp"

extern Configure config;

using namespace boost::interprocess;


void _messageQueue::createQueue(std::string name,int rw){
    //read
    if(rw==READ_MESSAGE){
        _mq=new message_queue(open_only,name.c_str());
    }
        //write
    else if(rw==WRITE_MESSAGE){
        message_queue::remove(name.c_str());

        _mq=new message_queue(create_only,name.c_str(),_messageQueueCnt, _messageQueueUnitSize);
    }
}

_messageQueue::_messageQueue(){
    _messageQueueCnt=config.getMessageQueueCnt();
    _messageQueueUnitSize=config.getMessageQueueUnitSize();
}

_messageQueue::_messageQueue(std::string name,int rw){
    _messageQueueCnt=config.getMessageQueueCnt();
    _messageQueueUnitSize=config.getMessageQueueUnitSize();
    createQueue(name,rw);
}

_messageQueue::~_messageQueue(){
    if(_mq!=NULL)
        delete _mq;
}