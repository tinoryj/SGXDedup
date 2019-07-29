#ifndef GENERALDEDUPSYSTEM_CHUNK_HPP
#define GENERALDEDUPSYSTEM_CHUNK_HPP

#include "configure.hpp"
#include <bits/stdc++.h>

using namespace std;

// system basic data structures
typedef struct {
    uint32_t ID;
    int type;
    int logicDataSize;
    u_char logicData[MAX_CHUNK_SIZE];
    u_char chunkHash[CHUNK_HASH_SIZE];
    u_char encryptKey[CHUNK_ENCRYPT_KEY_SIZE];
} Chunk_t;

typedef struct {
    vector<string> chunkHash;
    vector<string> logicData;
} StorageChunkList_t;

typedef struct {
    int logicDataSize;
    u_char logicData[MAX_CHUNK_SIZE];
    u_char chunkHash[CHUNK_HASH_SIZE];
} StorageCoreData_t;

typedef struct {
    uint32_t ID;
    int logicDataSize;
    u_char logicData[MAX_CHUNK_SIZE];
} RetrieverData_t;

typedef struct {
    uint32_t chunkID;
    int chunkSize;
    u_char chunkHash[CHUNK_HASH_SIZE];
    u_char chunkKey[CHUNK_ENCRYPT_KEY_SIZE];
} RecipeEntry_t;

typedef vector<Chunk_t> ChunkList_t;
typedef vector<RecipeEntry_t> RecipeList_t;

typedef struct {
    uint64_t fileSize;
    u_char fileNameHash[FILE_NAME_HASH_SIZE];
    uint64_t totalChunkNumber;
} FileRecipeHead_t;

typedef struct {
    uint64_t fileSize;
    u_char fileNameHash[FILE_NAME_HASH_SIZE];
    uint64_t totalChunkKeyNumber;
} KeyRecipeHead_t;

typedef struct {
    FileRecipeHead_t fileRecipeHead;
    KeyRecipeHead_t keyRecipeHead;
} Recipe_t;

typedef struct {
    union {
        Chunk_t chunk;
        Recipe_t recipe;
    };
    int dataType;
} Data_t;

typedef struct {
    u_char originHash[CHUNK_HASH_SIZE];
    u_char key[CHUNK_ENCRYPT_KEY_SIZE];
} KeyGenEntry_t;

// network data structures
typedef struct {
    int fd;
    int epfd;
    int type;
    int cid;
    int dataSize;
    u_char data[EPOLL_MESSAGE_DATA_SIZE];
} EpollMessage_t;

typedef struct {
    int fd;
    int epfd;
    u_char hash[CHUNK_HASH_SIZE];
    u_char key[CHUNK_ENCRYPT_KEY_SIZE];
} Message_t;

typedef struct {
    int messageType;
    int clientID;
    int dataSize;
} NetworkHeadStruct_t;

// database data structures

typedef struct {
    //key: string _chunkHash;
    //value: containerName, offset in container, chunk size;
    string containerName;
    uint32_t offset;
    uint32_t length;
} keyValueForChunkHash_t;

typedef struct {
    //key: string _filename;
    //value: file recipe name, key recipe name, version;
    string fileRecipeName;
    string keyRecipeName;
    uint32_t version;
} keyValueForFilename_t;

//dedup core data structures

typedef struct {
    vector<string> hashList;
    vector<string> chunks;
    std::chrono::system_clock::time_point startTime;
    int outDataTime;
} signedHashList_t;

typedef vector<uint32_t> RequiredChunk_t;

#endif //GENERALDEDUPSYSTEM_CHUNK_HPP
