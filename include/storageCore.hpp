#ifndef GENERALDEDUPSYSTEM_STORAGECORE_HPP
#define GENERALDEDUPSYSTEM_STORAGECORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataSR.hpp"
#include "dataStructure.hpp"
#include "database.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "socket.hpp"
#include <bits/stdc++.h>

using namespace std;

class Container {
public:
    uint32_t used_ = 0;
    char body_[4 << 20]; //4 M container size
    Container() {}
    ~Container() {}
    bool saveTOFile(string fileName);
};

class StorageCore {
private:
    DataSR* dataSRObj_;
    std::string lastContainerFileName_;
    std::string lastFileRecipeFileName_;
    std::string lastkeyRecipeFileName_;

    std::string containerNamePrefix_;
    std::string containerNameTail_;

    std::string fileRecipeNamePrefix_;
    std::string fileRecipeNameTail_;

    std::string keyRecipeNamePrefix_;
    std::string keyRecipeNameTail_;

    CryptoPrimitive* cryptoObj_;

    Container currentContainer_;

    bool writeContainer(keyValueForChunkHash_t& key, char* data);
    bool readContainer(keyValueForChunkHash_t key, char* data);

public:
    StorageCore(DataSR* dataSRObjTemp);
    ~StorageCore();

    void run();
    void chunkStoreThread();

    bool saveRecipe(std::string& recipeName, Recipe_t& fileRecipe, std::string& keyRecipe);
    bool restoreRecipe(std::string recipeName, Recipe_t& fileRecipe, std::string& keyRecipe);
    bool saveChunk(std::string chunkHash, char* chunkData, int chunkSize);
    bool restoreChunk(std::string chunkHash, std::string& chunkData);

    bool verifyRecipe(Recipe_t recipe, int version);
    void sendRecipe(std::string recipeName, int version, int fd, int epfd);

    bool createContainer();

    bool extractMQFromDataSR(EpollMessage_t& newMessage);
    bool insertMQToDataSR_CallBack(EpollMessage_t& newMessage);
    bool extractMQFromTimer(StorageCoreData_t& newData);
};

#endif