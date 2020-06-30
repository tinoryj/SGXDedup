#ifndef SGXDEDUP_MESSAGEQUEUE_HPP
#define SGXDEDUP_MESSAGEQUEUE_HPP

#include "configure.hpp"
#include "dataStructure.hpp"
#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

template <class T>
class messageQueue {
    boost::lockfree::queue<T, boost::lockfree::capacity<3000>> lockFreeQueue_;

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
        if (done_ == true) {
            return true;
        } else {
            return false;
        }
    }
    bool isEmpty()
    {
        return lockFreeQueue_.empty();
    }
};

#endif //SGXDEDUP_MESSAGEQUEUE_HPP