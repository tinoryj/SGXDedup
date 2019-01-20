//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP
#define GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "configure.hpp"
#include "chunk.hpp"
#include "message.hpp"
#include "seriazation.hpp"

#define READ_MESSAGE 0
#define WRITE_MESSAGE 1

class _messageQueue {
private:
    boost::interprocess::message_queue *_mq;
    unsigned long _messageQueueCnt,_messageQueueUnitSize;

public:
    _messageQueue();
    _messageQueue(std::string name,int rw);
    ~_messageQueue();
    void createQueue(std::string name,int rw);
    void push(Chunk data);
    void pop(Chunk &ans);
    void push(message data);
    void pop(message &ans);
    void push(epoll_message &data);
    void pop(epoll_message &data);
};
#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP
