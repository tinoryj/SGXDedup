//
// Created by a on 11/17/18.
//

//have bug when last mq did now remove from system when program close

#ifndef GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP
#define GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP

#define READ_MESSAGE 0
#define WRITE_MESSAGE 1

/* Chunk */
#define CHUNKER_TO_KEYCLIENT_MQ             "MQ0"

/* Chunk */
#define KEYCLIENT_TO_ENCODER_MQ             "MQ1"

/* Chunk */
#define ENCODER_TO_POW_MQ                   "MQ2"

/* networkStruct
 *      chunkList
 */
#define SENDER_IN_MQ                        "MQ3"

/* networkStruct
 *      chunkList
 *      sgx_msg
 *      Required ChunkID
 *      status_msg
 *      Recipe
 */
#define DATASR_IN_MQ                        "MQ4"

/*
 * sgx_msg
 * */
#define DATASR_TO_POWSERVER_MQ              "MQ5"


/*
 * signed Hash
 * chunkList
 * */
#define DATASR_TO_DEDUPCORE_MQ              "MQ6"

/*
 * Recipe
 * */
#define DATASR_TO_STORAGECORE_MQ            "MQ7"

/*
 * ChunkList
 * */
#define DEDUPCORE_TO_STORAGECORE_MQ         "MQ8"

#define RECEIVER_TO_DECODER_MQ              "MQ9"

#define DECODER_TO_RETRIEVER                "MQ10"

#define KEYMANGER_SR_TO_KEYGEN              "MQ11"

#define POWSERVER_TO_DEDUPCORE_MQ           "MQ12"

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

#include "configure.hpp"
#include "chunk.hpp"
#include "message.hpp"
#include "seriazation.hpp"

using namespace boost::interprocess;

void initMQForClient();
void initMQForServer();
void initMQForKeyServer();

class _messageQueue {
private:
    message_queue *_mq;
    unsigned long _messageQueueCnt, _messageQueueUnitSize;
    string _mqName;

public:
    _messageQueue();

    _messageQueue(std::string name, int rw);

    ~_messageQueue();

    void createQueue(std::string name, int rw);

    template<class T>
    void push(T data) {
        using namespace boost::posix_time;

        string tmp;
        serialize(data, tmp);
        bool status = false;
        while (true) {
            ptime abs_time = microsec_clock::universal_time() + boost::posix_time::milliseconds(5);
            status = _mq->timed_send(&tmp[0], tmp.length(), 0, abs_time);
            if (status) {
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }

    template<class T>
    bool pop(T &ans) {
        string tmp;
        unsigned int priority;
        message_queue::size_type recvd_size;
        tmp.resize(_messageQueueUnitSize);

        using namespace boost::posix_time;

        ptime abs_time = microsec_clock::universal_time() + boost::posix_time::milliseconds(5);
        bool status = _mq->timed_receive(&tmp[0], _messageQueueUnitSize,
                                         recvd_size, priority, abs_time);
        if (!status) return false;
    //    _mq->receive(&tmp[0], _messageQueueUnitSize, recvd_size, priority);
        tmp.resize(recvd_size);
        deserialize(tmp, ans);
        return true;
    }
};

#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP