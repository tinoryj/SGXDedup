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

public:

    Recipe_t *_recipe;

    Chunk();

    Chunk(uint64_t ID, uint64_t type = 0, uint64_t logicDataSize = 0, std::string logicData = "", \
                std::string metaData = "", std::string chunkHash = "");

    ~Chunk() {};

    uint64_t getID();

    uint64_t getType();

    uint64_t getLogicDataSize();

    std::string getLogicData();

    std::string getChunkHash();

    std::string getMetaData();

    std::string getEncryptKey();

    bool editType(uint64_t type);

//      bool editLogicData(std::string newLogicData);
//      bool editLogicDataSize(uint64_t newSize);
    bool editLogicData(std::string newLogicDataConten, uint64_t newLogicSize);

    bool editEncryptKey(std::string newKey);
    // any additional function of chunk


    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _ID;
        ar & _type;
        ar & _logicDataSize;
        ar & _logicData;
        ar & _metaData;
        ar & _chunkHash;
        ar & _encryptKey;
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


typedef vector<int> RequiredChunk;

#endif //GENERALDEDUPSYSTEM_CHUNK_HPP
