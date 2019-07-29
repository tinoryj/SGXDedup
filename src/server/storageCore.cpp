#include "storageCore.hpp"

extern Configure config;
extern Database fp2ChunkDB;
extern Database fileName2metaDB;

StorageCore::StorageCore(DataSR* dataSRObjTemp)
{
    dataSRObj_ = dataSRObjTemp;
    //timerObj_ = dataSRObj_->timer_;
    fileRecipeNamePrefix_ = config.getFileRecipeRootPath();
    keyRecipeNamePrefix_ = config.getKeyRecipeRootPath();
    containerNamePrefix_ = config.getContainerRootPath();
    fileRecipeNameTail_ = ".fileRecipe";
    keyRecipeNameTail_ = ".keyRecipe";
    containerNameTail_ = ".container";
    ifstream fin;
    fin.open(".StorageConfig", ifstream::in);
    if (fin.is_open()) {
        fin >> lastFileRecipeFileName_;
        fin >> lastkeyRecipeFileName_;
        fin >> lastContainerFileName_;
        fin >> currentContainer_.used_;
        fin.close();

        //read last container
        fin.open("cr/" + lastContainerFileName_ + containerNameTail_, ifstream::in | ifstream::binary);
        fin.read(currentContainer_.body_, currentContainer_.used_);
        fin.close();

    } else {
        lastkeyRecipeFileName_ = lastContainerFileName_ = lastFileRecipeFileName_ = "abcdefghijklmno";
        currentContainer_.used_ = 0;
    }
    cryptoObj_ = new CryptoPrimitive();
}

StorageCore::~StorageCore()
{
    ofstream fout;
    fout.open(".StorageConfig", ofstream::out);
    fout << lastFileRecipeFileName_ << endl;
    fout << lastkeyRecipeFileName_ << endl;
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
    return dataSRObj_->extractTimerMQToStorageCore(newData);
}

/* StorageCore thread handle
 * support four type of operation
 *      upload chunk
 *      download chunk
 *      upload recipe
 *      download recipe
 */
void StorageCore::run()
{
    EpollMessage_t epollMessage;
    NetworkHeadStruct_t networkHead;
    ChunkList_t chunks;
    Recipe_t recipe;
    StorageCoreData_t tmpChunk;
    while (1) {
        bool status = extractMQFromDataSR(epollMessage);
        if (status) {
            memcpy(&networkHead, epollMessage.data, sizeof(NetworkHeadStruct_t));
            switch (epollMessage.type) {
            case CLIENT_DOWNLOAD_RECIPE: {
                int version = 1, len = networkHead.dataSize;
                //memcpy(&version, &networkHead._data[len - sizeof(int)], sizeof(int));
                //networkHead._data.resize(len - sizeof(int));
                boost::thread th(boost::bind(&StorageCore::sendRecipe,
                    this,
                    networkHead.data,
                    version,
                    epollMessage.fd,
                    epollMessage.epfd));
                th.detach();
                break;
            }
            case CLIENT_UPLOAD_RECIPE: {
                int version = 1, len = networkHead.dataSize;

                deserialize(networkHead.data, recipe);
                cerr << "StorageCore : recv Recipe from client" << endl;
                cerr << "StorageCore : verify Recipe" << endl;
                if (!this->verifyRecipe(recipe, version)) {
                    networkHead.messageType = ERROR_RESEND;
                } else {
                    networkHead.messageType = SUCCESS;
                }
                networkHead.data.clear();
                serialize(*networkHead, epollMessage.data);
                insertMQToDataSR_CallBack(epollMessage);
                break;
            }

                /* omit this part
                     * chunk will send to client after server receive client's
                     * download recipe request
                     */
            case CLIENT_DOWNLOAD_CHUNK: {

                break;
            }
            default: {
            }
            }
        }
    }
}

/* StorageCore thread handle
 * support four type of operation
 *      upload chunk
 *      download chunk
 *      upload recipe
 *      download recipe
 */
void StorageCore::chunkStoreThread()
{
    StorageCoreData_t tempChunk;
    while (true) {
        if (extractMQFromTimer(tempChunk)) {
            string hashStr((char*)tempChunk.chunkHash, CHUNK_HASH_SIZE);
            if (!saveChunk(hashStr, (char*)tempChunk.logicData, tempChunk.logicDataSize)) {
                cerr << "StorageCore : save chunk error" << endl;
            }
        }
    }
}

bool StorageCore::saveRecipe(std::string& recipeName, Recipe_t& fileRecipe, std::string& keyRecipe)
{
    ofstream fileRecipeOut, keyRecipeOut;
    string writeRecipeName, buffer;

    writeRecipeName = fileRecipeNamePrefix_ + lastFileRecipeFileName_ + fileRecipeNameTail_;
    fileRecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);

    writeRecipeName = keyRecipeNamePrefix_ + lastkeyRecipeFileName_ + keyRecipeNameTail_;
    keyRecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);

    if (!(fileRecipeOut.is_open() && keyRecipeOut.is_open())) {
        std::cerr << "Can not open Recipe file: " << writeRecipeName << endl;
        return false;
    }

    serialize(fileRecipe, buffer);
    fileRecipeOut.write(&buffer[0], buffer.length());

    keyRecipeOut.write(&keyRecipe[0], keyRecipe.length());
    fileRecipeOut.close();
    keyRecipeOut.close();

    recipeName = lastkeyRecipeFileName_ = lastFileRecipeFileName_;
    next_permutation(lastFileRecipeFileName_.begin(), lastFileRecipeFileName_.end());
    //next_permutation(lastkeyRecipeFileName_.begin(),lastkeyRecipeFileName_.end());

    return true;
}

bool StorageCore::restoreRecipe(std::string recipeName, Recipe_t& fileRecipe, string& keyRecipe)
{
    ifstream fileRecipeIn, keyRecipeIn;
    string readRecipeName;

    readRecipeName = fileRecipeNamePrefix_ + lastkeyRecipeFileName_ + fileRecipeNameTail_;
    fileRecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);

    readRecipeName = keyRecipeNamePrefix_ + lastkeyRecipeFileName_ + keyRecipeNameTail_;
    keyRecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);

    if (!(fileRecipeIn.is_open() && keyRecipeIn.is_open())) {
        std::cerr << "StorageCore : Can not open Recipe file : " << readRecipeName;
        return false;
    }

    string buffer;
    int len;

    fileRecipeIn.seekg(0, fileRecipeIn.end);
    len = fileRecipeIn.tellg();
    fileRecipeIn.seekg(0, fileRecipeIn.beg);
    cerr << "StorageCore : fileRecipe have " << setbase(10) << len << " bytes " << endl;
    buffer.resize(len);
    fileRecipeIn.read(&buffer[0], len);
    deserialize(buffer, fileRecipe);

    keyRecipeIn.seekg(0, keyRecipeIn.end);
    len = keyRecipeIn.tellg();
    keyRecipeIn.seekg(0, keyRecipeIn.beg);
    cerr << "StorageCore : keyRecipe have " << setbase(10) << len << " bytes " << endl;
    keyRecipe.resize(len);
    keyRecipeIn.read(&keyRecipe[0], len);

    fileRecipeIn.close();
    keyRecipeIn.close();
}

bool StorageCore::saveChunk(std::string chunkHash, char* chunkData, int chunkSize)
{
    keyValueForChunkHash_t key;
    key.length = chunkSize;
    bool status = writeContainer(key, chunkData);
    if (!status) {
        std::cerr << "Error write container" << endl;
        return status;
    }

    string dbValue;
    memcpy(&dbValue[0], &key, sizeof(keyValueForChunkHash_t));
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
    keyValueForChunkHash_t key;
    string ans;
    bool status = fp2ChunkDB.query(chunkHash, ans);
    if (status) {
        memcpy(&key, &ans[0], sizeof(keyValueForChunkHash_t));
        char chunkData[key.length];
        if (readContainer(key, chunkData)) {
            memcpy(&chunkDataStr[0], chunkData, key.length);
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

//maybe should check all chunk in recipe had been storage in container
//reason: make sure no fake recipe

bool StorageCore::verifyRecipe(Recipe_t recipe, int version)
{
    Recipe_t f = recipe._f;
    string k = recipe._kencrypted;
    string recipeName, fileRecipeBuffer, keyBuffer;
    keyValueForFilename key;
    if (!this->saveRecipe(recipeName, f, k)) {
        std::cerr << "StorageCore : save recipe failed " << endl;
        return false;
    }
    std::cerr << "StorageCore : save recipe success" << endl;
    key.set(f._fileName, f._fileName, version);
    serialize(key, keyBuffer);
    fileName2metaDB.insert(keyBuffer, recipeName);
    return true;
}

// send recipe and all chunk in recipe in this function
void StorageCore::sendRecipe(std::string fileName, int version, int fd, int epfd)
{

    NetworkHeadStruct_t sendBody(0, 0);
    cerr << "start to send recipe to client" << endl;

    EpollMessage_t epollMessage;
    epollMessage.fd = fd;
    epollMessage.epfd = epfd;

    keyValueForFilename key;
    string recipeName, keyBuffer;
    key.set(fileName, fileName, version);
    serialize(key, keyBuffer);

    //error msg should be sent to client
    if (fileName2metaDB.query(keyBuffer, recipeName) == false) {
        std::cerr << "Can not file recipe" << endl;
        sendBody._type = ERROR_CLOSE;
        serialize(sendBody, epollMessage._data);
        this->insertMQ(epollMessage);
        return;
    }

    //send recipe
    Recipe_t f;
    string k;
    this->restoreRecipe(recipeName, f, k);

    Recipe_t r;
    r._kencrypted = k;
    r._f = f;

    serialize(r, sendBody._data);
    sendBody._type = SUCCESS;

    serialize(sendBody, epollMessage._data);
    this->insertMQ(epollMessage);

    //send chunks
    ChunkList_t chunks;
    string chunkData;
    struct sockaddr_in tmpaddr;
    Socket tmpsock(fd, tmpaddr);
    int cnt = config.getSendChunkBatchSize();
    sendBody._type = CLIENT_DOWNLOAD_CHUNK;
    for (auto it : f.body_) {
        cnt--;
        string hash(it._chunkHash, 32);
        if (!this->restoreChunk(hash, chunkData)) {
            cerr << "StorageCore : can not locate chunk " << endl;
            return;
        }
        chunks._chunks.push_back(chunkData);
        chunks._FP.push_back(it._chunkHash);
        if (cnt == 0) {
            cnt = config.getSendChunkBatchSize();
            serialize(chunks, sendBody._data);
            serialize(sendBody, epollMessage._data);

            //this->insertMQ(epollMessage);
            tmpsock.Send(epollMessage._data);

            chunks.clear();
        }
    }

    //send tail chunks
    if (chunks._FP.empty())
        return;
    serialize(chunks, sendBody._data);
    serialize(sendBody, epollMessage._data);

    //this->insertMQ(epollMessage);
    tmpsock.Send(epollMessage._data);
}

bool StorageCore::writeContainer(keyValueForChunkHash_t& key, char* data)
{
    if (key.length + currentContainer_.used_ < (4 << 20)) {
        memcpy(&currentContainer_.body_[currentContainer_.used_], data, key.length);
        key.containerName = lastContainerFileName_;
    } else {
        string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
        currentContainer_.saveTOFile(writeContainerName);
        next_permutation(lastContainerFileName_.begin(), lastContainerFileName_.end());
        currentContainer_.used_ = 0;
        memcpy(&currentContainer_.body_[currentContainer_.used_], data, key.length);
        key.containerName = lastContainerFileName_;
    }
    key.offset = currentContainer_.used_;
    return true;
}

bool StorageCore::readContainer(keyValueForChunkHash_t key, char* data)
{
    ifstream containerIn;
    string readName = containerNamePrefix_ + key.containerName + containerNameTail_;
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

bool StorageCore::createContainer() {}

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