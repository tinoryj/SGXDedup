#ifndef GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP
#define GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP

#define READ_MESSAGE 0
#define WRITE_MESSAGE 1

#include "boost/atomic.hpp"
#include "boost/lockfree/queue.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/thread.hpp"

#include "configure.hpp"
#include "dataStructure.hpp"

void initMQForClient();
void initMQForServer();
void initMQForKeyServer();

template <class T>
class messageQueue {
public:
    boost::lockfree::queue<T, boost::lockfree::capacity<sizeof(T)>> lockFreeQueue;
    boost::atomic<bool> done(false);
    messageQueue();
    ~messageQueue() = default();
    bool push(T data)
    {
        return lockFreeQueue.push(data);
    }
    bool pop(T data)
    {
        return lockFreeQueue.pop(data);
    }
    bool setJobDoneFlag()
    {
        done = true;
    }
};

#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP