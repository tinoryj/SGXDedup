#include "storageCore.hpp"
#include <sys/time.h>

struct timeval timestartStorage;
struct timeval timeendStorage;

extern Configure config;
extern Database fp2ChunkDB;
extern Database fileName2metaDB;

void PRINT_BYTE_ARRAY_STORAGE_CORE(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

StorageCore::StorageCore()
{
    RecipeNamePrefix_ = config.getRecipeRootPath();
    containerNamePrefix_ = config.getContainerRootPath();
    RecipeNameTail_ = ".recipe";
    containerNameTail_ = ".container";
    ifstream fin;
    fin.open(".StorageConfig", ifstream::in);
    if (fin.is_open()) {
        fin >> lastContainerFileName_;
        fin >> currentContainer_.used_;
        fin.close();

        //read last container
        fin.open(containerNamePrefix_ + lastContainerFileName_ + containerNameTail_, ifstream::in | ifstream::binary);
        fin.read(currentContainer_.body_, currentContainer_.used_);
        fin.close();

    } else {
        lastContainerFileName_ = "abcdefghijklmno";
        currentContainer_.used_ = 0;
    }
    cryptoObj_ = new CryptoPrimitive();
}

StorageCore::~StorageCore()
{
    ofstream fout;
    fout.open(".StorageConfig", ofstream::out);
    fout << lastContainerFileName_ << endl;
    fout << currentContainer_.used_ << endl;
    fout.close();

    string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
    currentContainer_.saveTOFile(writeContainerName);

    delete cryptoObj_;
}

bool StorageCore::saveChunks(NetworkHeadStruct_t& networkHead, char* data)
{
    // gettimeofday(&timestartStorage, NULL);
    int chunkNumber;
    memcpy(&chunkNumber, data, sizeof(int));
    int readSize = sizeof(int);
    for (int i = 0; i < chunkNumber; i++) {
        int currentChunkSize;
#ifdef STORAGE_SERVER_VERIFY_UPLOAD
        u_char oldHash[CHUNK_HASH_SIZE];
        memcpy(oldHash, data + readSize, CHUNK_HASH_SIZE);
#endif
        string originHash(data + readSize, CHUNK_HASH_SIZE);
        // cout << "save chunk hash" << endl;
        // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, &originHash[0], CHUNK_HASH_SIZE);
        readSize += CHUNK_HASH_SIZE;
        memcpy(&currentChunkSize, data + readSize, sizeof(int));
        readSize += sizeof(int);
#ifdef STORAGE_SERVER_VERIFY_UPLOAD
        u_char newHash[CHUNK_HASH_SIZE];
        cryptoObj_->generateHash((u_char*)data + readSize, currentChunkSize, newHash);
        if (memcmp(oldHash, newHash, CHUNK_HASH_SIZE) == 0) {
            if (!saveChunk(originHash, data + readSize, currentChunkSize)) {
                return false;
            }
        } else {
            cerr << "StorageCore : save chunk error, chunk may fake" << endl;
        }
#else
        if (!saveChunk(originHash, data + readSize, currentChunkSize)) {
            return false;
        }
#endif
        readSize += currentChunkSize;
    }
    // cerr << "DedupCore : recv " << setbase(10) << chunkNumber << " chunk from client" << endl;
    // gettimeofday(&timeendStorage, NULL);
    // long diff = 1000000 * (timeendStorage.tv_sec - timestartStorage.tv_sec) + timeendStorage.tv_usec - timestartStorage.tv_usec;
    // double second = diff / 1000000.0;
    // printf("save chunk list time is %ld us = %lf s\n", diff, second);
    return true;
}

bool StorageCore::restoreRecipeHead(char* fileNameHash, Recipe_t& restoreRecipe)
{
    string recipeName;
    string DBKey(fileNameHash, FILE_NAME_HASH_SIZE);
    if (fileName2metaDB.query(DBKey, recipeName)) {
        ifstream RecipeIn;
        string readRecipeName;
        readRecipeName = RecipeNamePrefix_ + recipeName + RecipeNameTail_;
        RecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);
        if (!RecipeIn.is_open()) {
            std::cerr << "StorageCore : Can not open Recipe file : " << readRecipeName;
            return false;
        } else {
            char recipeHeadBuffer[sizeof(Recipe_t)];
            memset(recipeHeadBuffer, 0, sizeof(Recipe_t));
            // RecipeIn.seekg(0, ios_base::end);
            // int nFileLen = RecipeIn.tellg();
            // cerr << "StorageCore : current recipe file size = " << nFileLen << endl;
            RecipeIn.seekg(ios::beg);
            RecipeIn.read(recipeHeadBuffer, sizeof(Recipe_t));
            RecipeIn.close();
            memcpy(&restoreRecipe, recipeHeadBuffer, sizeof(Recipe_t));
            // cerr << "StorageCore : restore file size = " << restoreRecipe.fileRecipeHead.fileSize << " total chunk number = " << restoreRecipe.fileRecipeHead.totalChunkNumber << endl;
            // PRINT_BYTE_ARRAY_STORAGE_CORE(stderr, recipeHeadBuffer, sizeof(Recipe_t));
            return true;
        }
    } else {
        return false;
    }
    return true;
}

bool StorageCore::saveRecipe(std::string recipeName, Recipe_t recipeHead, RecipeList_t recipeList, bool status)
{
    ofstream RecipeOut;
    string writeRecipeName, buffer;

    writeRecipeName = RecipeNamePrefix_ + recipeName + RecipeNameTail_;
    RecipeOut.open(writeRecipeName, ios::app | ios::binary);
    if (!RecipeOut.is_open()) {
        std::cerr << "Can not open Recipe file: " << writeRecipeName << endl;
        return false;
    }
    int recipeListSize = recipeList.size();
    if (status) {
        // char tempBuffer[sizeof(RecipeEntry_t)];
        for (int i = 0; i < recipeListSize; i++) {
            // memcpy(tempBuffer, &recipeList[i], sizeof(RecipeEntry_t));
            RecipeOut.write((char*)&recipeList[i], sizeof(RecipeEntry_t));
        }
    } else {
        char tempHeadBuffer[sizeof(Recipe_t)];
        memcpy(tempHeadBuffer, &recipeHead, sizeof(Recipe_t));
        RecipeOut.write(tempHeadBuffer, sizeof(Recipe_t));
        cerr << "StorageCore : save recipe head over, total chunk number = " << recipeHead.fileRecipeHead.totalChunkNumber << " file size = " << recipeHead.fileRecipeHead.fileSize << endl;
        // PRINT_BYTE_ARRAY_STORAGE_CORE(stderr, tempHeadBuffer, sizeof(Recipe_t));
        for (int i = 0; i < recipeListSize; i++) {
            // memcpy(tempBuffer, &recipeList[i], sizeof(RecipeEntry_t));
            RecipeOut.write((char*)&recipeList[i], sizeof(RecipeEntry_t));
        }
    }
    RecipeOut.close();
    return true;
}

bool StorageCore::restoreRecipeList(char* fileNameHash, char* recipeBuffer, uint32_t recipeBufferSize)
{
    ifstream RecipeIn;
    string readRecipeName;
    string recipeName;
    string DBKey(fileNameHash, FILE_NAME_HASH_SIZE);
    if (fileName2metaDB.query(DBKey, recipeName)) {
        ifstream RecipeIn;
        string readRecipeName;
        readRecipeName = RecipeNamePrefix_ + recipeName + RecipeNameTail_;
        RecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);
        if (!RecipeIn.is_open()) {
            std::cerr << "StorageCore : Can not open Recipe file : " << readRecipeName;
            return false;
        } else {
            Recipe_t currentFileRecipeHeader;
            RecipeIn.read((char*)&currentFileRecipeHeader, sizeof(Recipe_t));
            cerr << "StorageCore : restore file header, file size = " << currentFileRecipeHeader.fileRecipeHead.fileSize << ", total chunk number = " << currentFileRecipeHeader.fileRecipeHead.totalChunkNumber << endl;
            RecipeIn.read(recipeBuffer, recipeBufferSize);
            RecipeIn.close();
            if (RecipeIn.gcount() != recipeBufferSize) {
                cerr << "StorageCore : read file recipe error, should read " << recipeBufferSize << ", read size " << RecipeIn.gcount() << endl;
                return false;
            } else {
                // cerr << "StorageCore : read file recipe done, read " << recipeBufferSize << endl;
                return true;
            }
        }
    } else {
        return false;
    }
}

bool StorageCore::restoreChunks(char* recipeBuffer, uint32_t recipeBufferSize, uint32_t startID, uint32_t endID, ChunkList_t& restoredChunkList)
{
    for (int i = 0; i < endID - startID; i++) {
        RecipeEntry_t newRecipeEntry;
        memcpy(&newRecipeEntry, recipeBuffer + startID * sizeof(RecipeEntry_t) + i * sizeof(RecipeEntry_t), sizeof(RecipeEntry_t));
        string chunkHash((char*)newRecipeEntry.chunkHash, CHUNK_HASH_SIZE);
        string chunkData;
        // cerr << "StorageCore : restore chunk ID = " << newRecipeEntry.chunkID << ", size = " << newRecipeEntry.chunkSize << endl;
        // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, (char*)newRecipeEntry.chunkHash, CHUNK_HASH_SIZE);
        if (restoreChunk(chunkHash, chunkData)) {
            if (chunkData.length() != newRecipeEntry.chunkSize) {
                cerr << "StorageCore : restore chunk logic data size error" << endl;
                return false;
            } else {
                Chunk_t newChunk;
                newChunk.ID = newRecipeEntry.chunkID;
                newChunk.logicDataSize = newRecipeEntry.chunkSize;
                memcpy(newChunk.chunkHash, newRecipeEntry.chunkHash, CHUNK_HASH_SIZE);
                memcpy(newChunk.encryptKey, newRecipeEntry.chunkKey, CHUNK_ENCRYPT_KEY_SIZE);
                memcpy(newChunk.logicData, &chunkData[0], newChunk.logicDataSize);
                restoredChunkList.push_back(newChunk);
                // cerr << "StorageCore : restore chunk ID = " << newChunk.ID << endl;
                // cout << "StorageCore : Restore Chunk ID = " << newChunk.ID << " size = " << newChunk.logicDataSize << endl;
                // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, newChunk.chunkHash, CHUNK_HASH_SIZE);
                // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, newChunk.encryptKey, CHUNK_ENCRYPT_KEY_SIZE);
                // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, newChunk.logicData, newChunk.logicDataSize);
            }
        } else {
            cerr << "StorageCore : can not restore chunk" << endl;
            return false;
        }
    }
    return true;
}

bool StorageCore::saveChunk(std::string chunkHash, char* chunkData, int chunkSize)
{
    // cout << "Save Chunk size = " << chunkSize << endl;
    // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, &chunkHash[0], CHUNK_HASH_SIZE);
    // PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, chunkData, chunkSize);

    keyForChunkHashDB_t key;
    key.length = chunkSize;
    bool status = writeContainer(key, chunkData);
    if (!status) {
        std::cerr << "Error write container" << endl;
        return status;
    }

    string dbValue;
    dbValue.resize(sizeof(keyForChunkHashDB_t));
    memcpy(&dbValue[0], &key, sizeof(keyForChunkHashDB_t));
#ifdef STORAGE_SERVER_VERIFY_UPLOAD
    string ans;
    status = fp2ChunkDB.query(chunkHash, ans);
    if (status) {
        status = fp2ChunkDB.insert(chunkHash, dbValue);
        if (!status) {
            std::cerr << "StorageCore : Can't insert chunk to database" << endl;
            return false;
        } else {
            currentContainer_.used_ += key.length;
            return true;
        }
    } else {
        std::cerr << "StorageCore : Can't insert chunk to database, chunk is not valid" << endl;
        return false;
    }
#else
    status = fp2ChunkDB.insert(chunkHash, dbValue);
    if (!status) {
        std::cerr << "StorageCore : Can't insert chunk to database" << endl;
        return false;
    } else {
        currentContainer_.used_ += key.length;
        return true;
    }

#endif
}

bool StorageCore::restoreChunk(std::string chunkHash, std::string& chunkDataStr)
{
    keyForChunkHashDB_t key;
    string ans;
    bool status = fp2ChunkDB.query(chunkHash, ans);
    if (status) {
        memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
        // cerr << "StorageCore : restore chunk from " << key.containerName << ", size = " << key.length << endl;
        char chunkData[key.length];
        if (readContainer(key, chunkData)) {
            chunkDataStr.resize(key.length);
            memcpy(&chunkDataStr[0], chunkData, key.length);
            return true;
        } else {
            cerr << "StorageCore : can not read container for chunk" << endl;
            return false;
        }
    } else {
        cerr << "StorageCore : chunk not in database" << endl;
        return false;
    }
}

bool StorageCore::checkRecipeStatus(Recipe_t recipeHead, RecipeList_t recipeList)
{
    string recipeName;
    string DBKey((char*)recipeHead.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
    // cerr << "StorageCore : recv recpie head, file size = " << recipeHead.fileRecipeHead.fileSize << " total chunk number = " << recipeHead.fileRecipeHead.totalChunkNumber << endl;
    // PRINT_BYTE_ARRAY_STORAGE_CORE(stderr, recipeHead.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
    if (fileName2metaDB.query(DBKey, recipeName)) {
        cerr << "StorageCore : current file's recipe exist, add data back, recipe file name = " << recipeName << endl;
        if (!this->saveRecipe(recipeName, recipeHead, recipeList, true)) {
            std::cerr << "StorageCore : save recipe failed " << endl;
            return false;
        }
    } else {
        char recipeNameBuffer[FILE_NAME_HASH_SIZE * 2 + 1];
        for (int i = 0; i < FILE_NAME_HASH_SIZE; i++) {
            sprintf(recipeNameBuffer + 2 * i, "%02X", recipeHead.fileRecipeHead.fileNameHash[i]);
        }
        cerr << "StorageCore : current file's recipe not exist, new recipe file name = " << recipeNameBuffer << endl;
        string recipeNameNew(recipeNameBuffer, FILE_NAME_HASH_SIZE * 2);
        fileName2metaDB.insert(DBKey, recipeNameNew);
        if (!this->saveRecipe(recipeNameNew, recipeHead, recipeList, false)) {
            std::cerr << "StorageCore : save recipe failed " << endl;
            return false;
        }
    }
    cerr << "StorageCore : save recipe number = " << recipeList.size() << endl;
    return true;
}

bool StorageCore::writeContainer(keyForChunkHashDB_t& key, char* data)
{
    if (key.length + currentContainer_.used_ < (2 << 23)) {
        memcpy(&currentContainer_.body_[currentContainer_.used_], data, key.length);
        memcpy(key.containerName, &lastContainerFileName_[0], lastContainerFileName_.length());
    } else {
        string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
        currentContainer_.saveTOFile(writeContainerName);
        next_permutation(lastContainerFileName_.begin(), lastContainerFileName_.end());
        currentContainer_.used_ = 0;
        memcpy(&currentContainer_.body_[currentContainer_.used_], data, key.length);
        memcpy(key.containerName, &lastContainerFileName_[0], lastContainerFileName_.length());
    }
    key.offset = currentContainer_.used_;
    return true;
}

bool StorageCore::readContainer(keyForChunkHashDB_t key, char* data)
{
    ifstream containerIn;
    string containerNameStr((char*)key.containerName, lastContainerFileName_.length());
    string readName = containerNamePrefix_ + containerNameStr + containerNameTail_;

    if (containerNameStr.compare(lastContainerFileName_) == 0) {
        memcpy(data, currentContainer_.body_ + key.offset, key.length);
        return true;
    } else {
#ifdef STORAGE_READ_CACHE
        bool cacheHitStatus = containerCache.existsInCache(containerNameStr);
        if (cacheHitStatus) {
            string containerDataStr = containerCache.getFromCache(containerNameStr);
            memcpy(data, &containerDataStr[0] + key.offset, key.length);
            return true;
        } else {
            containerIn.open(readName, std::ifstream::in | std::ifstream::binary);
            if (!containerIn.is_open()) {
                std::cerr << "StorageCore : Can not open Container: " << readName << endl;
                return false;
            }
            containerIn.seekg(0, ios_base::end);
            int containerSize = containerIn.tellg();
            containerIn.seekg(0, ios_base::beg);
            containerIn.read(currentReadContainer_.body_, containerSize);
            containerIn.close();
            if (containerIn.gcount() != containerSize) {
                cerr << "StorageCore : read container " << readName << " error, should read " << containerSize << ", read in size " << containerIn.gcount() << endl;
                return false;
            }
            memcpy(data, currentReadContainer_.body_ + key.offset, key.length);
            string containerDataStrTemp(currentReadContainer_.body_, containerSize);
            containerCache.insertToCache(containerNameStr, containerDataStrTemp);
            return true;
        }
#else
        if (currentReadContainerFileName_.compare(containerNameStr) == 0) {
            memcpy(data, currentReadContainer_.body_ + key.offset, key.length);
            return true;
        } else {
            containerIn.open(readName, std::ifstream::in | std::ifstream::binary);
            if (!containerIn.is_open()) {
                std::cerr << "StorageCore : Can not open Container: " << readName << endl;
                return false;
            }
            containerIn.seekg(0, ios_base::end);
            int containerSize = containerIn.tellg();
            containerIn.seekg(0, ios_base::beg);
            containerIn.read(currentReadContainer_.body_, containerSize);
            containerIn.close();
            if (containerIn.gcount() != containerSize) {
                cerr << "StorageCore : read container " << readName << " error, should read " << containerSize << ", read in size " << containerIn.gcount() << endl;
                return false;
            }
            memcpy(data, currentReadContainer_.body_ + key.offset, key.length);
            currentReadContainerFileName_ = containerNameStr;
            return true;
        }
#endif
    }
}

bool Container::saveTOFile(string fileName)
{
    ofstream containerOut;
    containerOut.open(fileName, std::ofstream::out | std::ofstream::binary);
    if (!containerOut.is_open()) {
        cerr << "Can not open Container file : " << fileName << endl;
        return false;
    }
    containerOut.write(this->body_, this->used_);
    cerr << "Container : save " << setbase(10) << this->used_ << " bytes to file system" << endl;
    containerOut.close();
    return true;
}