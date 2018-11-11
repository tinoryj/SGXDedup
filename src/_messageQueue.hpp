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

class _messageQueue {
private:
	message_queue *_mq;

public:
	messageQueue();
	messageQueue(std::string name,int rw,int cnt=100,int unitSize=4000);
	~messageQueue();
	void createQueue(std::string name,int rw,int cnt,int unitSize);
	void push(Chunk data);
	void pop(Chunk &ans);
};

#endif