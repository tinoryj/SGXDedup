#include "_storage.hpp"

extern Configure config;

extern database _fp2ChunkDB;
extern database _fileName2metaDB;

_StorageCore::_StorageCore() {
    _crypto = new CryptoPrimitive(SHA256_TYPE);
    _inputMQ.createQueue(DEDUPCORE_TO_STORAGECORE_MQ, READ_MESSAGE);
    _outputMQ.createQueue(DATASR_IN_MQ, WRITE_MESSAGE);
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
}

_StorageCore::~_StorageCore() {
    ofstream fout;
    fout.open(".StorageConfig",ofstream::out);
    fout<<_lastFileRecipeFileName<<endl;
    fout<<_lastkeyRecipeFileName<<endl;
    fout<<_lastContainerFileName<<endl;
    fout<<_currentContainer._used<<endl;
    delete _crypto;
}

void _StorageCore::run() {
    epoll_message *msg=new epoll_message();
    Chunk tmpChunk;
    while(1){
        _inputMQ.pop(*msg);
        switch(msg->_data[0]){
            case CLIENT_UPLOAD_CHUNK:{
                string chunkHash,chunk=msg->_data.substr(1);
                _crypto->generaHash(chunk,chunkHash);
                if(saveChunk(chunkHash,chunk)){
                    msg->_data[0]=OK;
                }else{
                    msg->_data[0]=ERROR_RESEND;
                }
                _outputMQ.push(*msg);
                break;
            }
            case CLIENT_DOWNLOAD_CHUNK:{
                string chunkHash=msg->_data.substr(1),chunk;
                restoreChunk(chunkHash,chunk);
                msg->_data.resize(1);
                msg->_data.append(chunk);
                _outputMQ.push(*msg);
                break;
            }
            case CLIENT_UPLOAD_RECIPE:{
                boost::thread(boost::bind(_StorageCore::verifyRecipe,this,msg->_data.substr(1)));
                break;
            }
            case CLIENT_DOWNLOAD_RECIPE:{
                string fileRecipe,keyRecipe;
                if(restoreRecipe(msg->_data.substr(1),fileRecipe,keyRecipe)){
                    msg->_data[0]=OK;
                }
                else msg->_data[0]=ERROR_CLOSE;
                msg->_data.resize(1+sizeof(int));
                int len;

                len=fileRecipe.length();
                memcpy(&msg->_data[1],&len,sizeof(int));
                msg->_data+=fileRecipe;

                msg->_data.resize(msg->_data.length()+sizeof(int));

                len=keyRecipe.length();
                memcpy(&msg->_data[msg->_data.length()-sizeof(int)],&len,sizeof(int));
                msg->_data+=keyRecipe;

                _outputMQ.push(*msg);
                break;
            }
            default:{
                msg->_data[0]=ERROR_RESEND;
                msg->_data.resize(1);
                _outputMQ.push(*msg);
            }
        }
    }
}

bool _StorageCore::readContainer(keyValueForChunkHash key, std::string &data) {
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

bool _StorageCore::writeContainer(keyValueForChunkHash &key, std::string &data) {
    key._length = data.length();
    if (key._length + _currentContainer._used < (4 << 20)) {
        memcpy(&_currentContainer._body[_currentContainer._used], &data[0], key._length);
        key._containerName = _lastContainerFileName;
    } else {
        std::ofstream fout;
        string writeContainerName=_containerNamePrefix+_lastContainerFileName+_containerNameTail;
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

bool _StorageCore::restoreRecipe(std::string recipeName, std::string &fileRecipe, std::string &keyRecipe) {
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

    fileRecipeIn >> fileRecipe;
    keyRecipeIn >> keyRecipe;

    fileRecipeIn.close();
    keyRecipeIn.close();
}

bool _StorageCore::saveRecipe(std::string &recipeName, std::string &fileRecipe, std::string &keyRecipe) {
    ofstream fileRecipeOut, keyRecipeOut;
    string writeRecipeName;

    writeRecipeName=_fileRecipeNamePrefix+_lastFileRecipeFileName+_fileRecipeNameTail;
    fileRecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);

    writeRecipeName=_keyRecipeNamePrefix+_lastkeyRecipeFileName+_keyRecipeNameTail;
    keyRecipeOut.open(writeRecipeName, std::ofstream::out | std::ofstream::binary);

    if (!(fileRecipeOut.is_open() && keyRecipeOut.is_open())) {
        std::cerr << "Can not open Recipe file: "<<writeRecipeName<<endl;
        return false;
    }

    fileRecipeOut << fileRecipe;
    keyRecipeOut << keyRecipe;
    fileRecipeOut.close();
    keyRecipeOut.close();

    recipeName = _lastkeyRecipeFileName = _lastFileRecipeFileName;
    next_permutation(_lastFileRecipeFileName.begin(), _lastFileRecipeFileName.end());
    //next_permutation(_lastkeyRecipeFileName.begin(),_lastkeyRecipeFileName.end());

    return true;
}

bool _StorageCore::saveChunk(std::string chunkHash, std::string &chunkData) {
    keyValueForChunkHash key;
    bool ok=writeContainer(key,chunkData);
    if(!ok){
        std::cerr<<"Error write container\n";
        return ok;
    }

    string value=serialize(key);
    ok=_fp2ChunkDB.insert(chunkHash,value);
    if(!ok){
        std::cerr<<"Can insert chunk to database\n";
    }

    _currentContainer._used += key._length;

    return  ok;
}


bool _StorageCore::restoreChunk(std::string chunkHash, std::string &chunkData) {
    keyValueForChunkHash key;
    string ans;
    bool haveData = _fp2ChunkDB.query(chunkHash, ans);
    if (haveData) {
        key = deserialize(ans);
        haveData = readContainer(key, ans);
        if (haveData) {
            chunkData = deserialize(ans);
        }
    }
    return haveData;
}
