//
// Created by a on 1/30/19.
//

#include "reciver.hpp"

extern Configure config;

receiver::receiver() {
    _socket.init(CLIENTSIDE,config.getStorageServerIP(),config.getStorageServerPort());
}

receiver::~receiver() {
    _socket.finish();
}

void receiver::receiveChunk() {
    string tmp;
    Chunk tmpChunk;
    while(1){
        _socket.Recv(tmp);
        deserialize(tmp,tmpChunk);
        this->insertMQ(tmpChunk);
    }
}

void receiver::run(std::string fileName) {
    networkStruct request(CLIENT_DOWNLOAD_RECIPE, config.getClientID()), respond(0, 0);
    request._data = fileName;
    string reqBuffer,resBuffer;
    serialize(request, reqBuffer);
    while (1) {
        _socket.Send(reqBuffer);
        _socket.Recv(resBuffer);
        deserialize(resBuffer,respond);
        if(respond._type==ERROR_RESEND) continue;
        if(respond._type==ERROR_CLOSE){
            std::cerr<<"Server reject!\n";
            exit(1);
        }
        if(respond._type==SUCCESS)   break;
    }
    this->insertMQ(respond._data);

    int i,maxThread=config.getMaxThreadLimits();
    for(i=0;i<maxThread;i++){
        boost::thread th(boost::bind(&receiver::receiveChunk,this));
        th.detach();
    }
}

/*
receiver::receiver() {
    _outputMQ=this->getOutputMQ();
    _socket.init(CLIENTSIDE,config.getStorageServerIP(),config.getStorageServerPort());
    readyForChunkDownload=false;
}

receiver::~receiver() {
    _socket.finish();
}

bool receiver::receiveData(string &respond, int &status) {
    networkStruct respondBody(0,0);
    string tmp;
    {
        boost::unique_lock<boost::shared_mutex> t(this->_socketMtx);
        _socket.Recv(tmp);
    }
    deserialize(tmp,respondBody);
    status=respondBody._type;
    respond=respondBody._data;
    if(status==OK) return true;
    return false;
}

bool receiver::getFileRecipe(fileRecipe &respond, int &status) {
    string tmp;
    this->receiveData(tmp,status);
    deserialize(tmp,respond);
    if(status==OK)  return true;
    return false;
}

bool receiver::getKeyRecipe(string &respond, int &status) {
    this->receiveData(respond,status);
    if(status==OK) return true;
    return false;
}
 */