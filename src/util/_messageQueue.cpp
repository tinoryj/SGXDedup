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

void _messageQueue::push(Chunk data){
    std::string buffer;
    buffer=serialize(data);
    _mq->send(buffer.c_str(),sizeof(char)*buffer.length(),0);
}

void _messageQueue::pop(Chunk &ans){
    unsigned int priority;
    message_queue::size_type recvd_size;
    char buffer[_messageQueueUnitSize];
    _mq->receive(buffer,sizeof(buffer),recvd_size,priority);
    std::string str(buffer);
    ans=deserialize(str);
}

void _messageQueue::push(message data){
    _mq->send(&data,sizeof(data),0);
}

void _messageQueue::pop(message &ans){
    unsigned int priority;

#ifdef DEBUG
    std::cout<<sizeof(ans)<<endl<<sizeof(ans.con.fd)<<endl;
    std::cout<<sizeof(ans.con.sslsocket)<<endl<<sizeof(ans.epfd)<<endl;
    std::cout<<sizeof(ans.hash)<<endl<<sizeof(ans.key)<<endl;
#endif

    message_queue::size_type recvd_size;
    _mq->receive(&ans,sizeof(ans),recvd_size,priority);
}
