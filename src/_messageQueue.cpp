#include "_messageQueue.hpp"

void messageQueue::push(Chunk data){
	string buffer=serialize(data);
	mq->send(buffer.c_str(),sizeof(char)*buffer.length(),0);
}

void messageQueue::pop(Chunk &ans){
	unsigned int priority;  
	message_queue::size_type recvd_size; 
	char buffer[unitSize];
	mq.receive(buffer,sizeof(buffer),recvd_size,priority);
	ans=deserialize(buffer);
}

void messageQueue::createQueue(std::string name,int rw,int cnt,int unitSize){
	//read
	if(rw==READ_MESSAGE){
		_mq=new message_queue(boost::interprocess::open_only,name)
	}
	//write
	else if(rw==WRITE_MESSAGE){
		message_queue::remove(name);

		//dup
		_mq=new message_queue(boost::interprocess::create_only,
			name,cnt, unitSize);
	}
}

messageQueue::messageQueue(){}

messageQueue::messageQueue()(std::string name,int rw,int cnt,int unitSize){
	createQueue(name,rw,cnt,unitSize);
}

messageQueue::~messageQueue(){
	if(mq!=NULL)
		delete mq;
}
