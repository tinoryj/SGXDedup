#include "storageCore.hpp"

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

StorageCore::StorageCore(DataSR* dataSRObjTemp, Timer* timerObjTemp)
{
    dataSRObj_ = dataSRObjTemp;
    timerObj_ = timerObjTemp;
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
        fin.open("cr/" + lastContainerFileName_ + containerNameTail_, ifstream::in | ifstream::binary);
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

bool StorageCore::extractMQFromDataSR(EpollMessage_t& newMessage)
{
    return dataSRObj_->extractMQ2StorageCore(newMessage);
}

bool StorageCore::insertMQToDataSR_CallBack(EpollMessage_t& newMessage)
{
    return dataSRObj_->insertMQ2DataSR_CallBack(newMessage);
}

bool StorageCore::extractMQFromTimer(StorageCoreData_t& newData)
{
    return timerObj_->extractMQToStorageCore(newData);
}

void StorageCore::storageThreadForDataSR()
{
    EpollMessage_t epollMessage;
    ChunkList_t chunks;
    while (true) {
        if (extractMQFromDataSR(epollMessage)) {
            switch (epollMessage.type) {
            case CLIENT_DOWNLOAD_FILEHEAD: {
                Recipe_t restoredFileRecipe;
                if (restoreRecipeHead((char*)epollMessage.data, restoredFileRecipe)) {
                    epollMessage.type = SUCCESS;
                    epollMessage.dataSize = sizeof(Recipe_t);
                    memset(epollMessage.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    memcpy(epollMessage.data, &restoredFileRecipe, sizeof(Recipe_t));
                    cerr << "StorageCore : restored file recipe head for " << endl;
                    PRINT_BYTE_ARRAY_STORAGE_CORE(stderr, restoredFileRecipe.fileRecipeHead.fileNameHash, FILE_NAME_HASH_SIZE);
                    cerr << "\tfile size = " << restoredFileRecipe.fileRecipeHead.fileSize << " total chunk number = " << restoredFileRecipe.fileRecipeHead.totalChunkNumber << endl;

                    insertMQToDataSR_CallBack(epollMessage);
                } else {
                    epollMessage.type = ERROR_FILE_NOT_EXIST;
                    epollMessage.dataSize = 0;
                    memset(epollMessage.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    insertMQToDataSR_CallBack(epollMessage);
                }
                break;
            }
            case CLIENT_UPLOAD_RECIPE: {
                Recipe_t tempRecipeHead;
                memcpy(&tempRecipeHead, epollMessage.data, sizeof(Recipe_t));
                int currentRecipeEntryNumber = (epollMessage.dataSize - sizeof(Recipe_t)) / sizeof(RecipeEntry_t);
                RecipeList_t tempRecipeEntryList;
                for (int i = 0; i < currentRecipeEntryNumber; i++) {
                    RecipeEntry_t tempRecipeEntry;
                    memcpy(&tempRecipeEntry, epollMessage.data + sizeof(Recipe_t) + i * sizeof(RecipeEntry_t), sizeof(RecipeEntry_t));
                    tempRecipeEntryList.push_back(tempRecipeEntry);
                }
                cerr << "StorageCore : recv Recipe from client" << endl;

                if (!this->checkRecipeStatus(tempRecipeHead, tempRecipeEntryList)) {
                    cerr << "StorageCore : verify Recipe fail, send resend flag" << endl;
                    epollMessage.type = ERROR_RESEND;
                    epollMessage.dataSize = 0;
                    memset(epollMessage.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    insertMQToDataSR_CallBack(epollMessage);
                } else {
                    cerr << "StorageCore : verify Recipe succes" << endl;
                    epollMessage.type = SUCCESS;
                    epollMessage.dataSize = 0;
                    memset(epollMessage.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    insertMQToDataSR_CallBack(epollMessage);
                }
                break;
            }
            case CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE: {
                uint32_t startID, endID;
                ChunkList_t restoredChunkList;
                memcpy(&startID, epollMessage.data + FILE_NAME_HASH_SIZE, sizeof(uint32_t));
                memcpy(&endID, epollMessage.data + FILE_NAME_HASH_SIZE + sizeof(uint32_t), sizeof(uint32_t));
                cerr << "StorageCore : recv chunk download request, start ID = " << startID << " end ID = " << endID << endl;
                if (restoreRecipeAndChunk((char*)epollMessage.data, startID, endID, restoredChunkList)) {
                    cerr << "StorageCore : restore chunk from " << startID << " to " << endID << " over" << endl;
                    epollMessage.dataSize = restoredChunkList.size() * sizeof(Chunk_t);
                    epollMessage.type = SUCCESS;
                    memset(epollMessage.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    for (int i = 0; i < restoredChunkList.size(); i++) {
                        memcpy(epollMessage.data + i * sizeof(Chunk_t), &restoredChunkList[i], sizeof(Chunk_t));
                    }
                    insertMQToDataSR_CallBack(epollMessage);
                } else {
                    epollMessage.dataSize = 0;
                    epollMessage.type = ERROR_CHUNK_NOT_EXIST;
                    memset(epollMessage.data, 0, EPOLL_MESSAGE_DATA_SIZE);
                    insertMQToDataSR_CallBack(epollMessage);
                }
                break;
            }
            // case ERROR_CLIENT_CLOSE_CONNECT: {
            //     string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
            //     currentContainer_.saveTOFile(writeContainerName);
            //     break;
            // }
            default: {
                cerr << "StorageCore : recved message type error" << endl;
                break;
            }
            }
        }
    }
}

void StorageCore::storageThreadForTimer()
{
    StorageCoreData_t tempChunk;
    while (true) {
        if (extractMQFromTimer(tempChunk)) {
            string hashStr((char*)tempChunk.chunkHash, CHUNK_HASH_SIZE);
            if (!saveChunk(hashStr, (char*)tempChunk.logicData, tempChunk.logicDataSize)) {
                cerr << "StorageCore : save chunk error, current chunk hash = " << endl;
                PRINT_BYTE_ARRAY_STORAGE_CORE(stderr, tempChunk.chunkHash, CHUNK_HASH_SIZE);
            }
            // else {
            //     cerr << "StorageCore : save chunk success, current chunk size = " << tempChunk.logicDataSize << " hash = " << endl;
            //     PRINT_BYTE_ARRAY_STORAGE_CORE(stderr, tempChunk.chunkHash, CHUNK_HASH_SIZE);
            // }
        }
    }
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
            RecipeIn.read(recipeHeadBuffer, sizeof(Recipe_t));
            RecipeIn.close();
            memcpy(&restoreRecipe, recipeHeadBuffer, sizeof(Recipe_t));
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
    RecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);
    if (!RecipeOut.is_open()) {
        std::cerr << "Can not open Recipe file: " << writeRecipeName << endl;
        return false;
    }
    if (status == true) {
        RecipeOut.seekp(ios::end);
        char tempBuffer[sizeof(RecipeEntry_t)];
        for (int i = 0; i < recipeList.size(); i++) {
            memcpy(tempBuffer, &recipeList[i], sizeof(RecipeEntry_t));
            RecipeOut.write(tempBuffer, sizeof(RecipeEntry_t));
        }
    } else {
        char tempHeadBuffer[sizeof(Recipe_t)];
        memcpy(tempHeadBuffer, &recipeHead, sizeof(Recipe_t));
        RecipeOut.write(tempHeadBuffer, sizeof(Recipe_t));
        char tempBuffer[sizeof(RecipeEntry_t)];
        for (int i = 0; i < recipeList.size(); i++) {
            memcpy(tempBuffer, &recipeList[i], sizeof(RecipeEntry_t));
            RecipeOut.write(tempBuffer, sizeof(RecipeEntry_t));
        }
    }
    RecipeOut.close();
    return true;
}

bool StorageCore::restoreRecipeAndChunk(char* fileNameHash, uint32_t startID, uint32_t endID, ChunkList_t& restoredChunkList)
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
        }

        char readBuffer[sizeof(RecipeEntry_t) * (endID - startID)];
        RecipeIn.seekg(sizeof(Recipe_t) + startID * sizeof(RecipeEntry_t));
        RecipeIn.read(readBuffer, sizeof(RecipeEntry_t) * (endID - startID));
        RecipeIn.close();
        for (int i = 0; i < (endID - startID); i++) {
            RecipeEntry_t newRecipeEntry;
            memcpy(&newRecipeEntry, readBuffer + i * sizeof(RecipeEntry_t), sizeof(RecipeEntry_t));
            string chunkHash((char*)newRecipeEntry.chunkHash, CHUNK_HASH_SIZE);
            string chunkData;
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
                    //cerr << "StorageCore : restore chunk ID = " << newChunk.ID << endl;
                    cout << "Restore Chunk ID = " << newChunk.ID << " size = " << newChunk.logicDataSize << endl;
                    PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, newChunk.chunkHash, CHUNK_HASH_SIZE);
                    PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, newChunk.logicData, newChunk.logicDataSize);
                }

            } else {
                cerr << "StorageCore : can not restore chunk" << endl;
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool StorageCore::saveChunk(std::string chunkHash, char* chunkData, int chunkSize)
{

    cout << "Save Chunk size = " << chunkSize << endl;
    PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, &chunkHash[0], CHUNK_HASH_SIZE);
    PRINT_BYTE_ARRAY_STORAGE_CORE(stdout, chunkData, chunkSize);

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
    status = fp2ChunkDB.insert(chunkHash, dbValue);
    if (!status) {
        std::cerr << "Can't insert chunk to database" << endl;
        return false;
    } else {
        currentContainer_.used_ += key.length;
        return true;
    }
}

bool StorageCore::restoreChunk(std::string chunkHash, std::string& chunkDataStr)
{
    keyForChunkHashDB_t key;
    string ans;
    bool status = fp2ChunkDB.query(chunkHash, ans);
    if (status) {
        memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
        char chunkData[key.length];
        if (readContainer(key, chunkData)) {
            chunkDataStr.resize(key.length);
            memcpy(&chunkDataStr[0], chunkData, key.length);
            return true;
        } else {
            return false;
        }
    } else {
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
        if (!this->saveRecipe(recipeName, recipeHead, recipeList, true)) {
            std::cerr << "StorageCore : save recipe failed " << endl;
            return false;
        }
    } else {
        char recipeNameBuffer[FILE_NAME_HASH_SIZE * 2 + 1];
        for (int i = 0; i < FILE_NAME_HASH_SIZE; i++) {
            sprintf(recipeNameBuffer + 2 * i, "%02X", recipeHead.fileRecipeHead.fileNameHash[i]);
        }
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
    if (key.length + currentContainer_.used_ < (4 << 20)) {
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
        containerIn.open(readName, std::ifstream::in | std::ifstream::binary);

        if (!containerIn.is_open()) {
            std::cerr << "Can not open Container: " << readName << endl;
            return false;
        }

        containerIn.seekg(key.offset);
        containerIn.read(data, key.length);
        if (containerIn.gcount() == key.length) {
            return true;
        } else {
            return false;
        }
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