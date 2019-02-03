//
// Created by a on 2/3/19.
//

#ifndef GENERALDEDUPSYSTEM_STORAGECORE_HPP
#define GENERALDEDUPSYSTEM_STORAGECORE_HPP

#include "_storage.hpp"


struct _Container{
    uint32_t _used;
    char _body[4<<20];  //4 M
    _Container();
    ~_Container();
};


class storageCore:public _StorageCore{
private:

    _messageQueue _netRecvMQ;

    std::string _lastContainerFileName;
    std::string _lastFileRecipeFileName;
    std::string _lastkeyRecipeFileName;

    std::string _containerNamePrefix;
    std::string _containerNameTail;

    std::string _fileRecipeNamePrefix;
    std::string _fileRecipeNameTail;

    std::string _keyRecipeNamePrefix;
    std::string _keyRecipeNameTail;


    CryptoPrimitive *_crypto;

    _Container _currentContainer;

    bool writeContainer(keyValueForChunkHash &key,std::string &data);
    bool readContainer(keyValueForChunkHash key,std::string &data);

public:
    storageCore();
    ~storageCore();

    void run();

    bool saveRecipe(std::string &recipeName,fileRecipe_t &fileRecipe,std::string &keyRecipe);
    bool restoreRecipe(std::string recipeName,fileRecipe_t &fileRecipe,std::string &keyRecipe);
    bool saveChunk(std::string chunkHash,std::string &chunkData);
    bool restoreChunk(std::string chunkHash,std::string &chunkData);


    void verifyRecipe(Recipe_t recipe,int version);
    void sendRecipe(std::string recipeName,int version,int fd,int epfd);

    bool createContainer();
};

#endif