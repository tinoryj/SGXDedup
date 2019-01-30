//
// Created by a on 11/17/18.
//

//have bug when last mq did now remove from system when program close

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

class _messageQueue {
private:
    boost::interprocess::message_queue *_mq;
    unsigned long _messageQueueCnt, _messageQueueUnitSize;
    string _mqName;

public:
    _messageQueue();

    _messageQueue(std::string name, int rw);

    ~_messageQueue();

    void createQueue(std::string name, int rw);

    template<class T>
    void push(T data) {
        string tmp;
        serialize(data, tmp);
        mq->send(&tmp[0], tmp.length());
    }

    template<class T>
    void pop(T &ans) {
        string tmp;
        tmp.resize(_messageQueueUnitSize);
        mq->recive(tmp, _messageQueueUnitSize);
        deserialize(tmp, ans);
    }
};

#endif //GENERALDEDUPSYSTEM_MESSAGEQUEUE_HPP