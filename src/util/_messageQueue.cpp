#include "_messageQueue.hpp"

extern Configure config;

using namespace boost::interprocess;


void _messageQueue::createQueue(std::string name,int rw){
    //read
    if(rw==READ_MESSAGE){
        _mq=new message_queue(open_or_create,name.c_str(),_messageQueueCnt, _messageQueueUnitSize);
    }
        //write
    else if(rw==WRITE_MESSAGE){
        //message_queue::remove(name.c_str());

        _mq=new message_queue(open_or_create,name.c_str(),_messageQueueCnt, _messageQueueUnitSize);
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
    //if delete _mq here, will cause error when use temp object
    //so do not do that
/*
    if(_mq!=NULL)
        delete _mq;
*/
}


void initMQForClient(){
    message_queue::remove(CHUNKER_TO_KEYCLIENT_MQ);
    message_queue::remove(KEYCLIENT_TO_ENCODER_MQ);
    message_queue::remove(ENCODER_TO_POW_MQ);
    message_queue::remove(SENDER_IN_MQ);
    message_queue::remove(RECEIVER_TO_DECODER_MQ);
    message_queue::remove(DECODER_TO_RETRIEVER);
}

void initMQForServer(){
    message_queue::remove(DATASR_IN_MQ);
    message_queue::remove(DATASR_TO_POWSERVER_MQ);
    message_queue::remove(DATASR_TO_DEDUPCORE_MQ);
    message_queue::remove(DATASR_TO_STORAGECORE_MQ);
    message_queue::remove(DEDUPCORE_TO_STORAGECORE_MQ);
    message_queue::remove(POWSERVER_TO_DEDUPCORE_MQ);
}

void initMQForKeyServer(){
    message_queue::remove(KEYMANGER_SR_TO_KEYGEN);

}