//
// Created by a on 2/3/19.
//

#include "storageCore.hpp"
extern Configure config;
extern database fp2ChunkDB;
extern database fileName2metaDB;

storageCore::storageCore() {
    _fileRecipeNamePrefix = config.getFileRecipeRootPath();
    _keyRecipeNamePrefix = config.getKeyRecipeRootPath();
    _containerNamePrefix = config.getContainerRootPath();
    _fileRecipeNameTail = ".fileRecipe";
    _keyRecipeNameTail = ".keyRecipe";
    _containerNameTail = ".container";
    ifstream fin;
    fin.open(".StorageConfig", ifstream::in);
    if (fin.is_open()) {
        fin >> _lastFileRecipeFileName;
        fin >> _lastkeyRecipeFileName;
        fin >> _lastContainerFileName;
        fin >> _currentContainer._used;
        fin.close();
    } else {
        _lastkeyRecipeFileName =
        _lastContainerFileName =
        _lastFileRecipeFileName =
                "abcdefghijklmno";
        _currentContainer._used = 0;
    }
    _crypto = new CryptoPrimitive();
    _netRecvMQ.createQueue(DATASR_TO_STORAGECORE_MQ,READ_MESSAGE);
}

storageCore::~storageCore() {
    ofstream fout;
    fout.open(".StorageConfig",ofstream::out);
    fout<<_lastFileRecipeFileName<<endl;
    fout<<_lastkeyRecipeFileName<<endl;
    fout<<_lastContainerFileName<<endl;
    fout<<_currentContainer._used<<endl;
    delete _crypto;
}

/* storageCore thread handle
 * support four type of operation
 *      upload chunk
 *      download chunk
 *      upload recipe
 *      download recipe
 */
void storageCore::run() {
    epoll_message *msg1=new epoll_message();
    networkStruct *msg2=new networkStruct(0,0);
    chunkList chunks;
    Recipe_t recipe;
    Chunk tmpChunk;
    while(1){
        bool status=false;

        //wait for 500ms
        status=this->extractMQ(chunks);
        if(status){
            int size=(int)chunks._chunks.size();
            for(int i=0;i<size&&status;i++){
                status=saveChunk(chunks._FP[i],chunks._chunks[i]);
            }
            if(!status){
                std::cerr<<"Server error\n";
                std::cerr<<"Can not save chunks\n";
                exit(1);
            }

            //priority process data from dedupCore
            continue;
        }

        //wait for 500ms
        status=_netRecvMQ.pop(*msg1);
        if(status){
            deserialize(msg1->_data,*msg2);
            switch (msg2->_type){
                case CLIENT_DOWNLOAD_RECIPE:{
                    int version,len=msg2->_data.length();
                    memcpy(&version,&msg2->_data[len-sizeof(int)],sizeof(int));
                    msg2->_data.resize(len-sizeof(int));
                    boost::thread th(boost::bind(&storageCore::sendRecipe,this,
                                                 msg2->_data,
                                                 version,
                                                 msg1->_fd,
                                                 msg1->_epfd));
                    th.detach();
                    break;
                }
                case CLIENT_UPLOAD_RECIPE:{
                    int version,len=msg2->_data.length();
                    memcpy(&version,&msg2->_data[len-sizeof(int)],sizeof(int));
                    msg2->_data.resize(len-sizeof(int));
                    deserialize(msg2->_data,recipe);
                    this->verifyRecipe(recipe,version);
                    break;
                }

                    /* omit this part
                     * chunk will send to client after server receive client's
                     * download recipe request
                     */
                case CLIENT_DOWNLOAD_CHUNK:{

                    break;
                }
                default:{

                }
            }
        }
    }
}

bool storageCore::saveRecipe(std::string &recipeName, fileRecipe_t &fileRecipe, std::string &keyRecipe) {
    ofstream fileRecipeOut, keyRecipeOut;
    string writeRecipeName,buffer;

    writeRecipeName=_fileRecipeNamePrefix+_lastFileRecipeFileName+_fileRecipeNameTail;
    fileRecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);

    writeRecipeName=_keyRecipeNamePrefix+_lastkeyRecipeFileName+_keyRecipeNameTail;
    keyRecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);

    if (!(fileRecipeOut.is_open() && keyRecipeOut.is_open())) {
        std::cerr << "Can not open Recipe file: "<<writeRecipeName<<endl;
        return false;
    }

    serialize(fileRecipe,buffer);
    fileRecipeOut << buffer;

    keyRecipeOut << keyRecipe;
    fileRecipeOut.close();
    keyRecipeOut.close();

    recipeName = _lastkeyRecipeFileName = _lastFileRecipeFileName;
    next_permutation(_lastFileRecipeFileName.begin(), _lastFileRecipeFileName.end());
    //next_permutation(_lastkeyRecipeFileName.begin(),_lastkeyRecipeFileName.end());

    return true;
}

bool storageCore::restoreRecipe(std::string recipeName, fileRecipe_t &fileRecipe, string &keyRecipe) {
    ifstream fileRecipeIn, keyRecipeIn;
    string readRecipeName;

    readRecipeName = _fileRecipeNamePrefix + _lastkeyRecipeFileName + _fileRecipeNameTail;
    fileRecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);

    readRecipeName = _keyRecipeNamePrefix + _lastkeyRecipeFileName + _keyRecipeNameTail;
    keyRecipeIn.open(readRecipeName, ifstream::in | ifstream::binary);

    if (!(fileRecipeIn.is_open() && keyRecipeIn.is_open())) {
        std::cerr << "Can not open Recipe file: " << readRecipeName;
        return false;
    }

    string buffer;

    fileRecipeIn >> buffer;
    deserialize(buffer,fileRecipe);

    keyRecipeIn >> keyRecipe;

    fileRecipeIn.close();
    keyRecipeIn.close();
}

bool storageCore::saveChunk(std::string chunkHash, std::string &chunkData) {
    keyValueForChunkHash key;
    bool ok=writeContainer(key,chunkData);
    if(!ok){
        std::cerr<<"Error write container\n";
        return ok;
    }

    string value;
    serialize(key,value);
    ok=fp2ChunkDB.insert(chunkHash,value);
    if(!ok){
        std::cerr<<"Can insert chunk to database\n";
    }

    _currentContainer._used += key._length;

    return  ok;
}

bool storageCore::restoreChunk(std::string chunkHash, std::string &chunkData) {
    keyValueForChunkHash key;
    string ans;
    bool haveData = fp2ChunkDB.query(chunkHash, ans);
    if (haveData) {
        deserialize(ans,key);
        haveData = readContainer(key, ans);
        if (haveData) {
            deserialize(ans,chunkData);
        }
    }
    return haveData;
}

//maybe should check all chunk in recipe had been storage in container

void storageCore::verifyRecipe(Recipe_t recipe, int version) {
    fileRecipe_t f=recipe._f;
    string k=recipe._kencrypted;
    string recipeName,fileRecipeBuffer,keyRecipeBuffer,keyBuffer;
    keyValueForFilename key;
    if(!this->saveRecipe(recipeName,f,keyRecipeBuffer)){
        std::cerr<<"save recipe failed\n";
        exit(1);
    }
    key.set(f._fileName,f._fileName,version);
    serialize(key,keyBuffer);
    fileName2metaDB.insert(keyBuffer,recipeName);
}

// temp implement
// send recipe and all chunk in recipe in this function
void storageCore::sendRecipe(std::string fileName, int version,int fd,int epfd) {
    epoll_message msg1;
    msg1._fd=fd;
    msg1._epfd=epfd;

    keyValueForFilename key;
    string recipeName,keyBuffer;
    key.set(fileName,fileName,version);
    serialize(key,keyBuffer);

    //error msg should be sent to client
    if(fileName2metaDB.query(keyBuffer,recipeName)==false){
        std::cerr<<"Can not file recipe\n";
        exit(1);
    }

    //send recipe
    fileRecipe_t f;
    string k;
    this->restoreRecipe(recipeName,f,k);

    Recipe_t r;
    r._kencrypted=k;
    r._f=f;

    networkStruct sendBody(CLIENT_DOWNLOAD_RECIPE,0);
    serialize(r,sendBody._data);

    serialize(sendBody,msg1._data);
    this->insertMQ(msg1);

    //send chunks
    chunkList chunks;
    string chunkData;
    int cnt=config.getSendChunkBatchSize();
    sendBody._type=CLIENT_DOWNLOAD_CHUNK;
    for(auto it:f._body){
        cnt--;
        this->restoreChunk(it._chunkHash,chunkData);
        chunks._chunks.push_back(chunkData);
        chunks._FP.push_back(it._chunkHash);
        if(cnt==0){
            cnt=config.getSendChunkBatchSize();
            serialize(chunks,sendBody._data);
            serialize(sendBody,msg1._data);
            this->insertMQ(msg1);
            chunks.clear();
        }
    }

    //send tail chunks
    if(chunks._FP.empty()) return;
    serialize(chunks,sendBody._data);
    serialize(sendBody,msg1._data);
    this->insertMQ(msg1);
}
bool storageCore::writeContainer(keyValueForChunkHash &key, std::string &data) {
    key._length = data.length();
    if (key._length + _currentContainer._used < (4 << 20)) {
        memcpy(&_currentContainer._body[_currentContainer._used], &data[0], key._length);
        key._containerName = _lastContainerFileName;
    } else {
        std::ofstream fout;
        string writeContainerName = _containerNamePrefix + _lastContainerFileName + _containerNameTail;
        fout.open(writeContainerName, std::ofstream::out | std::ofstream::binary);
        if (!fout.is_open()) {
            std::cerr << "Can not open container: " << writeContainerName << endl;
            return false;
        }
        next_permutation(_lastContainerFileName.begin(), _lastContainerFileName.end());
        fout << _currentContainer._body;
        fout.close();
        _currentContainer._used = 0;
        memcpy(&_currentContainer._body[_currentContainer._used], &data[0], key._length);
        key._containerName = _lastContainerFileName;
    }
    key._offset = _currentContainer._used;
    return true;
}

bool storageCore::readContainer(keyValueForChunkHash key, std::string &data) {
    ifstream containerIn;
    string readName=_containerNamePrefix+key._containerName+_containerNameTail;
    containerIn.open(readName,std::ifstream::in|std::ifstream::binary);

    if(!containerIn.is_open()){
        std::cerr<<"Can not open Container: "<<readName<<endl;
        return false;
    }

    data.clear();
    data.resize(key._length);
    containerIn.seekg(key._offset);
    containerIn.read(&data[0],key._length);
    return true;
}

bool storageCore::createContainer() {}


_Container::_Container() {
    this->_used=0;
}

_Container::~_Container() {}