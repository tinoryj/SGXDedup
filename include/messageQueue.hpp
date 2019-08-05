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
public:
    boost::lockfree::queue<T, boost::lockfree::capacity<512>> lockFreeQueue_;
    boost::atomic<bool> done_;
    messageQueue()
    {
        done_ = false;
    }
    ~messageQueue() {}
    bool push(T& data)
    {
        return lockFreeQueue_.push(data);
    }
    bool pop(T& data)
    {
        return lockFreeQueue_.pop(data);
    }
    bool setJobDoneFlag()
    {
        done_ = true;
    }
};

#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP