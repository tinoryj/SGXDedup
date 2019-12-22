#ifndef GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP
#define GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

template <class T>
class messageQueue {
    boost::lockfree::queue<T, boost::lockfree::capacity<10000>> lockFreeQueue_;

public:
    boost::atomic<bool> done_;
    messageQueue()
    {
        done_ = false;
    }
    ~messageQueue()
    {
    }
    bool push(T& data)
    {
        while (!lockFreeQueue_.push(data))
            ;
        return true;
    }
    bool pop(T& data)
    {
        return lockFreeQueue_.pop(data);
    }
    bool setJobDoneFlag()
    {
        done_ = true;
    }
    bool isEmpty()
    {
        return lockFreeQueue_.empty();
    }
};

#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP