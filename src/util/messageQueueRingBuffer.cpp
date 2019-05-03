/*
 * BasicRingBuffer.hh
 * - a textbook-based ring buffer design
 *   based on William Stallings, "Operating Systems", 4 Ed, Pretice Hall.
 */

#ifndef __BasicRingBuffer_h__
#define __BasicRingBuffer_h__

#include "_messageQueue.hpp"
#include <pthread.h>

extern Configure config;

template <typename T>
class RingBuffer {
    typedef struct {
        int len;
        T data;
    } __attribute((packed)) Buffer_t;
    Buffer_t* buffer;
    volatile int _readIndex;
    volatile int _writeIndex;
    volatile int _currentBufferItemCount; // current number of occupied elements
    volatile int _maxBufferItemCount; // capacity of the buffer
    volatile int _run;
    volatile int _blockOnEmpty;
    pthread_mutex_t mAccess;
    pthread_cond_t cvEmpty;
    pthread_cond_t cvFull;

public:
    RingBuffer()
    {
        bufferMaxItemCount = config.getMessageQueueCnt();
        buffer = new Buffer_t[bufferMaxItemCount];
        _readIndex = 0;
        _writeIndex = 0;
        _currentBufferItemCount = 0;
        _maxBufferItemCount = bufferMaxItemCount;
        _run = true;
        _blockOnEmpty = true;
        pthread_mutex_init(&mAccess, NULL);
        pthread_cond_init(&cvEmpty, NULL);
        pthread_cond_init(&cvFull, NULL);
    }

    ~RingBuffer() = default();

    inline int nextVal(int x) 
    { 
        return (x + 1) >= _maxBufferItemCount ? 0 : x + 1; 
    }

    int Insert(T* data, int len)
    {
        pthread_mutex_lock(&mAccess);
        if (_currentBufferItemCount == _maxBufferItemCount) {
            pthread_cond_wait(&cvFull, &mAccess);
        }
        buffer[_writeIndex].len = len;
        memcpy(&(buffer[_writeIndex].data), data, len);
        _writeIndex = nextVal(_writeIndex);
        _currentBufferItemCount++;
        pthread_cond_signal(&cvEmpty);
        pthread_mutex_unlock(&mAccess);
        return 0;
    }

    int Extract(T* data)
    {
        pthread_mutex_lock(&mAccess);
        if (_currentBufferItemCount == 0) {
            if (!_blockOnEmpty) {
                pthread_cond_signal(&cvFull);
                pthread_mutex_unlock(&mAccess);
                return -1;
            }
            pthread_cond_wait(&cvEmpty, &mAccess);
        }
        memcpy(data, &(buffer[_readIndex].data), buffer[_readIndex].len);
        _readIndex = nextVal(_readIndex);
        _currentBufferItemCount--;
        pthread_cond_signal(&cvFull);
        pthread_mutex_unlock(&mAccess);
        return 0;
    }

    void StopWhenEmptied()
    {
        pthread_mutex_lock(&mAccess);
        _run = false;
        pthread_mutex_unlock(&mAccess);
        pthread_cond_signal(&cvEmpty);
    }
};

#endif
