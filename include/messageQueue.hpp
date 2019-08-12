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
    int capacity;
    boost::lockfree::queue<T> lockFreeQueue_;
    int getCapacity(int size)
    {
        capacity = size;
        return capacity;
    }

public:
    boost::atomic<bool> done_;
    explicit messageQueue(int size)
        : lockFreeQueue_(getCapacity(size))
    {
        done_ = false;
    }
    ~messageQueue()
    {
        lockFreeQueue_.~queue<T>();
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