#ifndef SGXDEDUP_MESSAGEQUEUE_HPP
#define SGXDEDUP_MESSAGEQUEUE_HPP

#if QUEUE_TYPE == QUEUE_TYPE_CONCURRENT_QUEUE
#include "concurrentqueue.h"
#endif
#if QUEUE_TYPE == QUEUE_TYPE_READ_WRITE_QUEUE
#include <readerwriterqueue/atomicops.h>
#include <readerwriterqueue/readerwriterqueue.h>
#endif
#include "configure.hpp"
#include "dataStructure.hpp"
#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#if QUEUE_TYPE == QUEUE_TYPE_CONCURRENT_QUEUE || QUEUE_TYPE == QUEUE_TYPE_READ_WRITE_QUEUE
using namespace moodycamel;
#endif

template <class T>
class messageQueue {
#if QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_SPSC_QUEUE
    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<5000>> lockFreeQueue_;
#elif QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_QUEUE
    boost::lockfree::queue<T, boost::lockfree::capacity<5000>> lockFreeQueue_;
#elif QUEUE_TYPE == QUEUE_TYPE_READ_WRITE_QUEUE
    ReaderWriterQueue<T, 5000> lockFreeQueue;
#elif QUEUE_TYPE == QUEUE_TYPE_CONCURRENT_QUEUE
    ConcurrentQueue<T>* lockFreeQueue = new ConcurrentQueue<T>(5000);
#endif
public:
    boost::atomic<bool> done_;
    messageQueue()
    {
        done_ = false;
    }
    ~messageQueue()
    {
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
#if QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_SPSC_QUEUE || QUEUE_TYPE == QUEUE_TYPE_LOCKFREE_QUEUE
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
    bool isEmpty()
    {
        return lockFreeQueue_.empty();
    }
#elif QUEUE_TYPE == QUEUE_TYPE_READ_WRITE_QUEUE
    bool push(T& data)
    {
        while (!lockFreeQueue.try_enqueue(data))
            ;
        return true;
    }
    bool pop(T& data)
    {
        return lockFreeQueue.try_dequeue(data);
    }
    bool isEmpty()
    {
        T* front = lockFreeQueue.peek();
        if (front == nullptr) {
            return true;
        } else {
            return false;
        }
    }
#elif QUEUE_TYPE == QUEUE_TYPE_CONCURRENT_QUEUE
    bool push(T& data)
    {
        while (!lockFreeQueue->try_enqueue(data))
            ;
        return true;
    }
    bool pop(T& data)
    {
        return lockFreeQueue->try_dequeue(data);
    }
    bool isEmpty()
    {
        int size = lockFreeQueue->size_approx();
        if (size == 0) {
            return true;
        } else {
            return false;
        }
    }
#endif
};

#endif //SGXDEDUP_MESSAGEQUEUE_HPP