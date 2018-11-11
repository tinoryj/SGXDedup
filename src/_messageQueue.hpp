#ifndef _MESSAGEQUEUE_HPP
#define _MESSAGEQUEUE_HPP

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "configure.hpp"
#include "chunk.hpp"
#include "seriazation.hpp"

#define READ_MESSAGE 0
#define WRITE_MESSAGE 1

class _messageQueue {
private:
	boost::interprocess::message_queue *_mq;
	int _messageQueueCnt,_messageQueueUnitSize;

public:
	_messageQueue();
	_messageQueue(std::string name,int rw);
	~_messageQueue();
	void createQueue(std::string name,int rw);
	void push(Chunk data);
	void pop(Chunk &ans);
};

#endif