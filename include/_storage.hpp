#include <string>

#include "_messageQueue.hpp"
#include "seriazation.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "database.hpp"
#include "message.hpp"
#include "protocol.hpp"
#include "CryptoPrimitive.hpp"
#include "Sock.hpp"
#include <algorithm>
#include "tmp.hpp"
#include "recipe.hpp"
#include <boost/thread.hpp>

#define STORAGEHISTORYFILE ".storage_history"


class _StorageCore {
private:
    _messageQueue _inputMQ;
    _messageQueue _outputMQ;



//  std::vector<database> _dbSet;
    std::vector<std::ifstream> _intputContainerSet;
    std::vector<std::ofstream> _outputContainerSet;

    // any additional info



public:
    _StorageCore();
    ~_StorageCore();
    bool insertMQ(epoll_message &msg);
    bool extractMQ(chunkList &chunks);
    virtual bool createContainer() = 0;
    virtual bool writeContainer(keyValueForChunkHash &key,std::string &data) = 0;
    virtual bool readContainer(keyValueForChunkHash key,std::string &data) = 0;
    _messageQueue getInputMQ();
    _messageQueue getOutputMQ();
//    std::vector<std::ifstream> getIntputContainerSet();
//    std::vector<std::ofstream> getOutputContainerSet();
    // any additional functions


};