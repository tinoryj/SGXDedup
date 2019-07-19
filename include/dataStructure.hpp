#ifndef GENERALDEDUPSYSTEM_CHUNK_HPP
#define GENERALDEDUPSYSTEM_CHUNK_HPP

#include "configure.hpp"

typedef struct {
    uint64_t ID;
    int type;
    int logicDataSize;
    u_char logicData[MAX_CHUNK_SIZE];
    u_char chunkHash[CHUNK_HASH_SIZE];
    u_char encryptKey[CHUNK_ENCRYPT_KEY_SIZE];
} Chunk_t;

typedef vector<Chunk_t> ChunkList_t;

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

typedef struct {
    int fd;
    int epfd;
    int type;
    int cid;
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
    string data;
} NetworkStruct_t;

#endif //GENERALDEDUPSYSTEM_CHUNK_HPP
