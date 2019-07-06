//
// Created by a on 11/17/18.
//

//have bug when last mq did now remove from system when program close

#ifndef GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP
#define GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP

#define READ_MESSAGE 0
#define WRITE_MESSAGE 1

/* Chunk */
#define CHUNKER_TO_KEYCLIENT_MQ             0

/* Chunk */
#define KEYCLIENT_TO_ENCODER_MQ             1

/* Chunk */
#define ENCODER_TO_POW_MQ                   2

/* networkStruct
 *      chunkList
 */
#define SENDER_IN_MQ                        3

/* networkStruct
 *      chunkList
 *      sgx_msg
 *      Required ChunkID
 *      status_msg
 *      Recipe
 */
#define DATASR_IN_MQ                        4

/*
 * sgx_msg
 * */
#define DATASR_TO_POWSERVER_MQ              5

/*
 * signed Hash
 * chunkList
 * */
#define DATASR_TO_DEDUPCORE_MQ              6
/*
 * Recipe
 * */
#define DATASR_TO_STORAGECORE_MQ            7

/*
 * ChunkList
 * */
#define DEDUPCORE_TO_STORAGECORE_MQ         8

#define RECEIVER_TO_DECODER_MQ              9

#define DECODER_TO_RETRIEVER                10

#define KEYMANGER_SR_TO_KEYGEN              11

#define POWSERVER_TO_DEDUPCORE_MQ           12

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/pool/pool.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

#include "configure.hpp"
#include "chunk.hpp"
#include "message.hpp"
#include "seriazation.hpp"
#include <thread>

using namespace boost::interprocess;

void initMQForClient();
void initMQForServer();
void initMQForKeyServer();

class _messageQueue {
private:
    message_queue *_mq;
    boost::pool<> *_alloc;
    unsigned long _messageQueueCnt, _messageQueueUnitSize,_messageQueueIndex;
    string _mqName;

public:
    _messageQueue();
    ~_messageQueue();

    _messageQueue(int index, int rw);

//    ~_messageQueue();

    void createQueue(int index, int rw);

    template<class T>
    void push(T data) {
        using namespace boost::posix_time;

        string tmp;
        serialize(data, tmp);
        if(tmp.length()>this->_messageQueueUnitSize){
            cerr<<setbase(10);
            cerr<<"_messageQueue: mq size overflow for mq "<<this->_messageQueueIndex<<
                "("<<tmp.length()<<">"<<this->_messageQueueUnitSize<<")"<<endl;
            exit(0);
        }
        bool status = false;
        while (true) {
            ptime abs_time = microsec_clock::universal_time() + boost::posix_time::microseconds(100);
            status = _mq->timed_send(&tmp[0], tmp.length(), 0, abs_time);
            if (status) {
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }


    template<class T>
    bool pop(T &ans) {
        void *tmp=this->_alloc->malloc();
        unsigned int priority;
        message_queue::size_type recvd_size = 0;

        using namespace boost::posix_time;

        ptime abs_time = microsec_clock::universal_time() + boost::posix_time::microseconds(100);
        bool status = _mq->timed_receive(tmp, _messageQueueUnitSize,recvd_size, priority, abs_time);
        if (!status){
            this->_alloc->free(tmp);
            return false;
        }
        deserialize(string((const char*)tmp,recvd_size), ans);
        this->_alloc->free(tmp);
        return true;
    }
};

#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP