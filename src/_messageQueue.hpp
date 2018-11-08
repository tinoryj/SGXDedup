#ifndef _MESSAGEQUEUE_HPP
#define _MESSAGEQUEUE_HPP

#include<cstdio>
#include<cstring>
#include<algorithm>
#include<iostream>
#include<string>
#include<vector>
#include<stack>
#include<bitset>
#include<cstdlib>
#include<cmath>
#include<set>
#include<list>
#include<deque>
#include<map>
#include<queue>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "configure.hpp"
#include "chunk.hpp"

#define READ_MESSAGE 0
#define WRITE_MESSAGE 1

template<class T>
class MessageQueue {
private:
	message_queue *_mq;
public:
	MessageQueue(std::string name,int rw,int cnt=100);
	~MessageQueue();
	void createQueue(std::string name,int rw,int cnt);
	void push(T data){
		void *p=(void*)&data;
		mq->send(&p,sizeof(data),0);
	}
	void pop(T &ans){
		unsigned int priority;  
		message_queue::size_type recvd_size; 
		void *p;
		mq.receive(&p,sizeof(void*),recvd_size,priority);
		ans=*((T*)p);
	}
};

void MessageQueue::createQueue(std::string name,int rw,int cnt){
	//read
	if(rw==READ_MESSAGE){
		_mq=new message_queue(boost::interprocess::open_only,name)
	}
	//write
	else if(rw==WRITE_MESSAGE){
		message_queue::remove(name);
		_mq=new message_queue(boost::interprocess::create_only,\
			name,cnt,sizeof(void*));
	}
}

MessageQueue::MessageQueue()(std::string name,int rw,int cnt){
	createQueue(name,rw,cnt);
}

MessageQueue::~MessageQueue(){
	if(mq!=NULL)
		delete mq;
}

#endif