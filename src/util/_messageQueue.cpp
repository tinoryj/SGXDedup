#include "_messageQueue.hpp"

extern Configure config;

using namespace boost::interprocess;


void _messageQueue::createQueue(int index,int rw) {
    this->_messageQueueIndex=index;
    string name;
    name.resize(20);
    int n = sprintf(&name[0], "mq%d", index);
    name.resize(n);
    _messageQueueCnt = config.getMessageQueueCnt(index);
    _messageQueueUnitSize = config.getMessageQueueUnitSize(index);
    this->_alloc=new boost::pool<>(this->_messageQueueUnitSize);
    if (rw == READ_MESSAGE) {
        _mq = new message_queue(open_or_create, name.c_str(), _messageQueueCnt, _messageQueueUnitSize);
    }
        //write
    else if (rw == WRITE_MESSAGE) {
        //message_queue::remove(name.c_str());

        _mq = new message_queue(open_or_create, name.c_str(), _messageQueueCnt, _messageQueueUnitSize);
    }
}

_messageQueue::_messageQueue(){

}

_messageQueue::_messageQueue(int index,int rw){
    createQueue(index,rw);
}
//
_messageQueue::~_messageQueue(){
//    //if delete _mq here, will cause error when use temp object
//    //so do not do that
///*
//    if(_mq!=NULL)
//        delete _mq;
//*/
}
//
//_messageQueue::~_messageQueue(){
//    //if delete _mq here, will cause error when use temp object
//    //so do not do that
///*
//    if(_mq!=NULL)
//        delete _mq;
//*/
//}


void initMQForClient(){
    int a[]={CHUNKER_TO_KEYCLIENT_MQ,KEYCLIENT_TO_ENCODER_MQ,
            ENCODER_TO_POW_MQ,SENDER_IN_MQ,
            RECEIVER_TO_DECODER_MQ,DECODER_TO_RETRIEVER};
    for (int i=0;i< sizeof(a)/ sizeof(int);i++){
        string name;
        name.resize(20);
        int n=sprintf(&name[0],"mq%d",a[i]);
        name.resize(n);
        message_queue::remove(name.c_str());
    }
}

void initMQForServer(){
    int a[]={DATASR_IN_MQ,DATASR_TO_POWSERVER_MQ,
             DATASR_TO_DEDUPCORE_MQ,DATASR_TO_STORAGECORE_MQ,
             DEDUPCORE_TO_STORAGECORE_MQ,POWSERVER_TO_DEDUPCORE_MQ};
    for (int i=0;i< sizeof(a)/ sizeof(int);i++){
        string name;
        name.resize(20);
        int n=sprintf(&name[0],"mq%d",a[i]);
        name.resize(n);
        message_queue::remove(name.c_str());
    }
}

void initMQForKeyServer(){
    int a[]={KEYMANGER_SR_TO_KEYGEN};
    for (int i=0;i< sizeof(a)/ sizeof(int);i++){
        string name;
        name.resize(20);
        int n=sprintf(&name[0],"mq%d",a[i]);
        name.resize(n);
        message_queue::remove(name.c_str());
    }
}