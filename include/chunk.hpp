//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_CHUNK_HPP
#define GENERALDEDUPSYSTEM_CHUNK_HPP

#include <string>
#include "recipe.hpp"

class Chunk {
private:
    uint64_t _ID;
    uint64_t _type;
    uint64_t _logicDataSize;
    std::string _logicData;
    std::string _metaData;
    std::string _chunkHash;
    std::string _encryptKey;
    // any additional info of chunk
    unsigned char _recipe[8];

public:

    Chunk();

    /* if do't set logicData as reference here, something strange happend
     * for example:
     * if message="aaa" then call new Chunk(..., message, ...),
     * when get int this construct function, logicData will not equal to message.
     * I DO NOT KNOW WHY ......................................................
     * */
    Chunk(uint64_t ID, uint64_t type, uint64_t logicDataSize, std::string &logicData, \
                std::string metaData = "", std::string chunkHash = "");

    ~Chunk() {};

    uint64_t getID();

    uint64_t getType();

    uint64_t getLogicDataSize();

    std::string getLogicData();

    std::string getChunkHash();

    std::string getMetaData();

    std::string getEncryptKey();

    Recipe_t *getRecipePointer(){
        Recipe_t *ans;
        memcpy(&ans,_recipe,sizeof(Recipe_t*));
        return ans;
    }

    void editRecipePointer(Recipe_t *p){
        memcpy(_recipe,&p,sizeof(Recipe_t*));
    }

    bool editType(uint64_t type);

//      bool editLogicData(std::string newLogicData);
//      bool editLogicDataSize(uint64_t newSize);
    bool editLogicData(std::string newLogicDataConten, uint64_t newLogicSize);

    bool editEncryptKey(std::string newKey);
    // any additional function of chunk

    //use to edit hash after encode chunk
    bool editChunkHash(std::string newHash);

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _ID;
        ar & _type;
        ar & _logicDataSize;
        ar & _logicData;
        ar & _metaData;
        ar & _chunkHash;
        ar & _encryptKey;
        ar & _recipe;
    }
};


struct chunkList {
    vector <string> _FP;
    vector <string> _chunks;

    void clear() {
        _FP.clear();
        _chunks.clear();
    }

    void push_back(Chunk &tmpChunk){
        _FP.push_back(tmpChunk.getChunkHash());
        _chunks.push_back(tmpChunk.getLogicData());
    }

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _FP;
        ar & _chunks;
    }
};



#endif //GENERALDEDUPSYSTEM_CHUNK_HPP
