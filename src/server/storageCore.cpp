#include "storageCore.hpp"

extern Configure config;
extern Database fp2ChunkDB;
extern Database fileName2metaDB;

storageCore::storageCore()
{
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

storageCore::~storageCore()
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

/* storageCore thread handle
 * support four type of operation
 *      upload chunk
 *      download chunk
 *      upload recipe
 *      download recipe
 */
void storageCore::run()
{
    EpollMessage_t* msg1;
    NetworkHeadStruct_t* msg2; //00
    ChunkList_t chunks;
    Recipe_t recipe;
    Chunk_t tmpChunk;
    while (1) {
        bool status = false;

        //wait for 5ms
        status = this->extractMQ(chunks);
        if (status) {
            int totalSaveSize = 0; //for debug

            int size = (int)chunks._chunks.size();
            for (int i = 0; i < size && status; i++) {
                status = saveChunk(chunks._FP[i], chunks._chunks[i]);
                totalSaveSize += chunks._chunks[i].length();
            }
            if (!status) {
                std::cerr << "storageCore :Server error" << endl;
                std::cerr << "stotageCore : Can not save chunks" << endl;
                exit(1);
            } else {
                std::cerr << "storageCore : save " << size << " chunks" << endl;
                std::cerr << "storageCore : total size is " << totalSaveSize << endl;
            }

            //priority process data from dedupCore
            continue;
        }

        //wait for 5ms
        status = netRecvMQ_.pop(msg1);
        if (status) {
            deserialize(msg1->_data, *msg2);
            switch (msg2->_type) {
            case CLIENT_DOWNLOAD_RECIPE: {
                int version = 1, len = msg2->_data.length();
                //memcpy(&version, &msg2->_data[len - sizeof(int)], sizeof(int));
                //msg2->_data.resize(len - sizeof(int));
                boost::thread th(boost::bind(&storageCore::sendRecipe, this,
                    msg2->_data,
                    version,
                    msg1->_fd,
                    msg1->_epfd));
                th.detach();
                break;
            }
            case CLIENT_UPLOAD_RECIPE: {
                int version = 1, len = msg2->_data.length();

                deserialize(msg2->_data, recipe);
                cerr << "storageCore : "
                     << "recv Recipe from client" << endl;
                cerr << "storageCore : "
                     << "verify Recipe" << endl;
                if (!this->verifyRecipe(recipe, version)) {
                    msg2->_type = ERROR_RESEND;
                } else {
                    msg2->_type = SUCCESS;
                }
                msg2->_data.clear();
                serialize(*msg2, msg1->_data);
                _netSendMQ.push(*msg1);
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

bool storageCore::saveRecipe(std::string& recipeName, fileRecipe_t& fileRecipe, std::string& keyRecipe)
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

bool storageCore::restoreRecipe(std::string recipeName, fileRecipe_t& fileRecipe, string& keyRecipe)
{
    ifstream fileRecipeIn, keyRecipeIn;
    string readRecipeName;

    readRecipeName = fileRecipeNamePrefix_ + lastkeyRecipeFileName_ + fileRecipeNameTail_;
    fileRecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);

    readRecipeName = keyRecipeNamePrefix_ + lastkeyRecipeFileName_ + keyRecipeNameTail_;
    keyRecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);

    if (!(fileRecipeIn.is_open() && keyRecipeIn.is_open())) {
        std::cerr << "storageCore : Can not open Recipe file: " << readRecipeName;
        return false;
    }

    string buffer;
    int len;

    fileRecipeIn.seekg(0, fileRecipeIn.end);
    len = fileRecipeIn.tellg();
    fileRecipeIn.seekg(0, fileRecipeIn.beg);
    cerr << "storageCore : fileRecipe have " << setbase(10) << len << " bytes" << endl;
    buffer.resize(len);
    fileRecipeIn.read(&buffer[0], len);
    deserialize(buffer, fileRecipe);

    keyRecipeIn.seekg(0, keyRecipeIn.end);
    len = keyRecipeIn.tellg();
    keyRecipeIn.seekg(0, keyRecipeIn.beg);
    cerr << "storageCore : keyRecipe have " << setbase(10) << len << " bytes" << endl;
    keyRecipe.resize(len);
    keyRecipeIn.read(&keyRecipe[0], len);

    fileRecipeIn.close();
    keyRecipeIn.close();
}

bool storageCore::saveChunk(std::string chunkHash, std::string& chunkData)
{
    keyValueForChunkHash key;
    bool ok = writeContainer(key, chunkData);
    if (!ok) {
        std::cerr << "Error write container" << endl;
        return ok;
    }

    string value;
    serialize(key, value);
    ok = fp2ChunkDB.insert(chunkHash, value);
    if (!ok) {
        std::cerr << "Can insert chunk to database" << endl;
    }

    currentContainer_.used_ += key._length;

    return ok;
}

bool storageCore::restoreChunk(std::string chunkHash, std::string& chunkData)
{
    keyValueForChunkHash key;
    string ans;
    bool haveData = fp2ChunkDB.query(chunkHash, ans);
    if (haveData) {
        deserialize(ans, key);
        haveData = readContainer(key, chunkData);
        return haveData;
    }
    return haveData;
}

//TODO: non-repudiation
//maybe should check all chunk in recipe had been storage in container
//reason: make sure no fake recipe

bool storageCore::verifyRecipe(Recipe_t recipe, int version)
{
    fileRecipe_t f = recipe._f;
    string k = recipe._kencrypted;
    string recipeName, fileRecipeBuffer, keyBuffer;
    keyValueForFilename key;
    if (!this->saveRecipe(recipeName, f, k)) {
        std::cerr << "storageCore : save recipe failed" << endl;
        return false;
    }
    std::cerr << "StorageCore : save recipe success" << endl;
    key.set(f._fileName, f._fileName, version);
    serialize(key, keyBuffer);
    fileName2metaDB.insert(keyBuffer, recipeName);
    return true;
}

// temp implement
// send recipe and all chunk in recipe in this function
void storageCore::sendRecipe(std::string fileName, int version, int fd, int epfd)
{

    NetworkHeadStruct_t sendBody(0, 0);
    cerr << "start to send recipe to client" << endl;

    EpollMessage_t msg1;
    msg1._fd = fd;
    msg1._epfd = epfd;

    keyValueForFilename key;
    string recipeName, keyBuffer;
    key.set(fileName, fileName, version);
    serialize(key, keyBuffer);

    //error msg should be sent to client
    if (fileName2metaDB.query(keyBuffer, recipeName) == false) {
        std::cerr << "Can not file recipe" << endl;
        sendBody._type = ERROR_CLOSE;
        serialize(sendBody, msg1._data);
        this->insertMQ(msg1);
        return;
    }

    //send recipe
    fileRecipe_t f;
    string k;
    this->restoreRecipe(recipeName, f, k);

    Recipe_t r;
    r._kencrypted = k;
    r._f = f;

    serialize(r, sendBody._data);
    sendBody._type = SUCCESS;

    serialize(sendBody, msg1._data);
    this->insertMQ(msg1);

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
            cerr << "storageCore : can not locate chunk" << endl;
            return;
        }
        chunks._chunks.push_back(chunkData);
        chunks._FP.push_back(it._chunkHash);
        if (cnt == 0) {
            cnt = config.getSendChunkBatchSize();
            serialize(chunks, sendBody._data);
            serialize(sendBody, msg1._data);

            //this->insertMQ(msg1);
            tmpsock.Send(msg1._data);

            chunks.clear();
        }
    }

    //send tail chunks
    if (chunks._FP.empty())
        return;
    serialize(chunks, sendBody._data);
    serialize(sendBody, msg1._data);

    //this->insertMQ(msg1);
    tmpsock.Send(msg1._data);
}

bool storageCore::writeContainer(keyValueForChunkHash& key, std::string& data)
{
    key._length = data.length();
    if (key._length + currentContainer_.used_ < (4 << 20)) {
        memcpy(&currentContainer_.body_[currentContainer_.used_], &data[0], key._length);
        key._containerName = lastContainerFileName_;
    } else {
        string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
        currentContainer_.saveTOFile(writeContainerName);
        next_permutation(lastContainerFileName_.begin(), lastContainerFileName_.end());
        currentContainer_.used_ = 0;
        memcpy(&currentContainer_.body_[currentContainer_.used_], &data[0], key._length);
        key._containerName = lastContainerFileName_;
    }
    key._offset = currentContainer_.used_;
    return true;
}

bool storageCore::readContainer(keyValueForChunkHash key, std::string& data)
{
    ifstream containerIn;
    string readName = containerNamePrefix_ + key._containerName + containerNameTail_;
    containerIn.open(readName, std::ifstream::in | std::ifstream::binary);

    if (!containerIn.is_open()) {
        std::cerr << "Can not open Container: " << readName << endl;
        return false;
    }

    data.clear();
    data.resize(key._length);
    containerIn.seekg(key._offset);
    containerIn.read(&data[0], key._length);
    int sz = containerIn.gcount();
    return true;
}

bool storageCore::createContainer() {}

_Container::_Container()
{
    this->used_ = 0;
}

_Container::~_Container() {}

void _Container::saveTOFile(string fileName)
{
    std::ofstream containerOut;
    containerOut.open(fileName, std::ofstream::out | std::ofstream::binary);
    if (!containerOut.is_open()) {
        std::cerr << "Can not open Container file : " << fileName << endl;
        return;
        ;
    }
    containerOut.write(this->body_, this->used_);
    cerr << "Container : "
         << "save " << setbase(10) << this->used_ << " bytes to file system" << endl;
    containerOut.close();
}