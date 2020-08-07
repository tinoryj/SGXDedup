#ifndef SGXDEDUP_STORAGECORE_HPP
#define SGXDEDUP_STORAGECORE_HPP

#if STORAGE_CORE_READ_CACHE == 1
#include "cache.hpp"
#endif
#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "database.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>

using namespace std;

class Container {
public:
    uint32_t used_ = 0;
    char body_[2 << 23]; //8 M container size
    Container() { }
    ~Container() { }
    bool saveTOFile(string fileName);
};

class StorageCore {
private:
    std::string lastContainerFileName_;
    std::string currentReadContainerFileName_;
    std::string containerNamePrefix_;
    std::string containerNameTail_;
    std::string RecipeNamePrefix_;
    std::string RecipeNameTail_;
    CryptoPrimitive* cryptoObj_;
    Container currentContainer_;
    Container currentReadContainer_;
    uint64_t maxContainerSize_;
    bool writeContainer(keyForChunkHashDB_t& key, char* data);
    bool readContainer(keyForChunkHashDB_t key, char* data);
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_WHOLE_RECIPE_FILE
    double queryDBTime = 0;
    double readContainerTime = 0;
    int readContainerNumber = 0;
    double queryDBTimeUpload = 0;
    double insertDBTimeUpload = 0;
    double writeContainerTime = 0;
    int uniqueChunkNumber = 0;
#endif
#if MULTI_CLIENT_UPLOAD_TEST == 1
    std::mutex mutexContainerOperation_;
#endif
public:
    StorageCore();
    ~StorageCore();
#if STORAGE_CORE_READ_CACHE == 1
    Cache containerCache;
#endif
#if RECIPE_MANAGEMENT_METHOD == ENCRYPT_ONLY_KEY_RECIPE_FILE
    bool saveChunks(NetworkHeadStruct_t& networkHead, char* data);
    bool saveRecipe(std::string recipeName, Recipe_t recipeHead, RecipeList_t recipeList, bool status);
    bool restoreChunks(char* recipeBuffer, uint32_t recipeBufferSize, uint32_t startID, uint32_t endID, ChunkList_t& restoredChunkList);
    bool restoreRecipeList(char* fileNameHash, char* recipeBuffer, uint32_t recipeBufferSize);
    bool saveChunk(std::string chunkHash, char* chunkData, int chunkSize);
    bool restoreChunk(std::string chunkHash, std::string& chunkData);
    bool checkRecipeStatus(Recipe_t recipeHead, RecipeList_t recipeList);
    bool restoreRecipeHead(char* fileNameHash, Recipe_t& restoreRecipe);
#elif RECIPE_MANAGEMENT_METHOD == ENCRYPT_WHOLE_RECIPE_FILE
    bool restoreChunks(NetworkHeadStruct_t& networkHead, char* data);
    bool storeRecipes(char* fileNameHash, u_char* recipeContent, uint64_t recipeSize);
    bool restoreRecipeAndChunk(RecipeList_t recipeList, uint32_t startID, uint32_t endID, ChunkList_t& restoredChunkList);
    bool storeChunk(string chunkHash, char* chunkData, int chunkSize);
    bool storeChunks(NetworkHeadStruct_t& networkHead, char* data);
    bool restoreChunk(std::string chunkHash, std::string& chunkDataStr);
    bool restoreRecipes(char* fileNameHash, u_char* recipeContent, uint64_t& recipeSize);
    bool restoreRecipesSize(char* fileNameHash, uint64_t& recipeSize);
#endif
};

#endif //SGXDEDUP_STORAGECORE_HPP