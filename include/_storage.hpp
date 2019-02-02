#include <string>

#include "_messageQueue.hpp"
#include "seriazation.hpp"
#include "configure.hpp"
#include "chunk.hpp"
#include "database.hpp"
#include "message.hpp"
#include "protocol.hpp"
#include "CryptoPrimitive.hpp"
#include "storeStruct.hpp"
#include "Sock.hpp"
#include <algorithm>
#include "tmp.hpp"
#include "recipe.hpp"
#include <boost/thread.hpp>

#define STORAGEHISTORYFILE ".storage_history"


class _StorageCore {
private:
    _messageQueue _inputMQFromDataSR;
    _messageQueue _inputMQFromDedupCore;
    _messageQueue _outputMQ;

    _Container _currentContainer;

    CryptoPrimitive *_crypto;

//  std::vector<database> _dbSet;
    std::vector<std::ifstream> _intputContainerSet;
    std::vector<std::ofstream> _outputContainerSet;

    // any additional info

    std::string _lastContainerFileName;
    std::string _lastFileRecipeFileName;
    std::string _lastkeyRecipeFileName;

    std::string _containerNamePrefix;
    std::string _containerNameTail;

    std::string _fileRecipeNamePrefix;
    std::string _fileRecipeNameTail;

    std::string _keyRecipeNamePrefix;
    std::string _keyRecipeNameTail;


public:
    _StorageCore();
    ~_StorageCore();
//    bool insertMQ();
//    bool extractMQ();
    virtual bool createContainer() = 0;
    virtual bool writeContainer(keyValueForChunkHash &key,std::string &data) = 0;
    virtual bool readContainer(keyValueForChunkHash key,std::string &data) = 0;
//    _messageQueue getInputMQ();
//    _messageQueue getOutputMQ();
//    std::vector<std::ifstream> getIntputContainerSet();
//    std::vector<std::ofstream> getOutputContainerSet();
    // any additional functions

    void run();
    bool saveRecipe(std::string &recipeName,fileRecipe_t &fileRecipe,std::string &keyRecipe);
    bool restoreRecipe(std::string recipeName,fileRecipe_t &fileRecipe,std::string &keyRecipe);
    bool saveChunk(std::string chunkHash,std::string &chunkData);
    bool restoreChunk(std::string chunkHash,std::string &chunkData);


    void verifyRecipe(Recipe_t recipe,int version);
    void sendRecipe(std::string recipeName,int version,int fd,int epfd);
};