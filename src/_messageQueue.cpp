#include "_messageQueue.hpp"

extern Configure config;

using namespace boost::interprocess;

void _messageQueue::push(Chunk data){
	std::string buffer=serialize(data);
	_mq->send(buffer.c_str(),sizeof(char)*buffer.length(),0);
}

void _messageQueue::pop(Chunk &ans){
	unsigned int priority;  
	message_queue::size_type recvd_size; 
	char buffer[_messageQueueUnitSize];
	_mq->receive(buffer,sizeof(buffer),recvd_size,priority);
	ans=deserialize(buffer);
}

void _messageQueue::createQueue(std::string name,int rw){
	//read
	if(rw==READ_MESSAGE){
		_mq=new message_queue(open_only,name);
	}
	//write
	else if(rw==WRITE_MESSAGE){
		message_queue::remove(name);

		_mq=new message_queue(create_only,name,_messageQueueCnt, _messageQueueUnitSize);
	}
}

_messageQueue::_messageQueue(){

}

_messageQueue::_messageQueue()(std::string name,int rw){
	createQueue(name,rw);
	_messageQueue_messageQueueCnt=config.getMessageQueue_messageQueueCnt();
	_messageQueueUnitSize=config.getMessageQueueUnitSize();
}

_messageQueue::~_messageQueue(){
	if(_mq!=NULL)
		delete _mq;
}
