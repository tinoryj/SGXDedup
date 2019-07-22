#ifndef GENERALDEDUPSYSTEM_STORAGECORE_HPP
#define GENERALDEDUPSYSTEM_STORAGECORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "database.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "socket.hpp"
#include <bits/stdc++.h>

using namespace std;

class Container {
    uint32_t used_;
    char body_[4 << 20]; //4 M container size
    Container();
    ~Container();
    void saveTOFile(string fileName);
};

class storageCore {
private:
    messageQueue<Chunk_t> netRecvMQ_;
    messageQueue<Chunk_t> netSendMQ_;

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

    bool writeContainer(keyValueForChunkHash& key, std::string& data);
    bool readContainer(keyValueForChunkHash key, std::string& data);

public:
    storageCore();
    ~storageCore();

    void run();

    bool saveRecipe(std::string& recipeName, FileRecipeHead_t& fileRecipe, std::string& keyRecipe);
    bool restoreRecipe(std::string recipeName, FileRecipeHead_t& fileRecipe, std::string& keyRecipe);
    bool saveChunk(std::string chunkHash, std::string& chunkData);
    bool restoreChunk(std::string chunkHash, std::string& chunkData);

    bool verifyRecipe(Recipe_t recipe, int version);
    void sendRecipe(std::string recipeName, int version, int fd, int epfd);

    bool createContainer();
};

#endif