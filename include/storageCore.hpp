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
    char body_[2 << 24]; //4 M container size
    Container() {}
    ~Container() {}
    bool saveTOFile(string fileName);
};

class StorageCore {
private:
    DataSR* dataSRObj_;
    std::string lastContainerFileName_;
    std::string containerNamePrefix_;
    std::string containerNameTail_;
    std::string RecipeNamePrefix_;
    std::string RecipeNameTail_;
    CryptoPrimitive* cryptoObj_;
    Container currentContainer_;
    bool writeContainer(keyForChunkHashDB_t& key, char* data);
    bool readContainer(keyForChunkHashDB_t key, char* data);

public:
    StorageCore(DataSR* dataSRObjTemp);
    ~StorageCore();

    void storageThreadForDataSR();
    bool saveChunks(EpollMessage_t& epollMessageTemp);
    bool saveRecipe(std::string recipeName, Recipe_t recipeHead, RecipeList_t recipeList, bool status);
    bool restoreRecipeAndChunk(char* fileNameHash, uint32_t startID, uint32_t endID, ChunkList_t& restoredChunkList);
    bool saveChunk(std::string chunkHash, char* chunkData, int chunkSize);
    bool restoreChunk(std::string chunkHash, std::string& chunkData);
    bool checkRecipeStatus(Recipe_t recipeHead, RecipeList_t recipeList);
    bool restoreRecipeHead(char* fileNameHash, Recipe_t& restoreRecipe);
    bool extractMQFromDataSR(EpollMessage_t& newMessage);
    bool insertMQToDataSR_CallBack(EpollMessage_t& newMessage);
};

#endif