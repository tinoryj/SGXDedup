#include "_messageQueue.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "protocol.hpp"
#include "message.hpp"
#include "boost/bind.hpp"
#include "boost/thread.hpp"
#include "CryptoPrimitive.hpp"
#include "database.hpp"
#include "time.h"
#include "chrono"
#include "thread"
#include <chrono>
#include <algorithm>
#include <queue>
#include <map>
#include "Socket.hpp"

#include "tmp.hpp"



class _DedupCore {
    private:
        _messageQueue _inputMQ;
        _messageQueue _outputMQ;


       //std::vector<leveldb::DB> _dbSet;
       // any additional info
public:
        _DedupCore();
        ~_DedupCore();
        bool insertMQ(epoll_message &msg);
        bool extractMQ(epoll_message &msg);
        virtual bool dataDedup() = 0;
        _messageQueue getInputMQ();
        _messageQueue getOutputMQ();
        //leveldb::DB getDBObject(int dbPosition);
        // any additional functions
};
