//
// Created by a on 1/12/19.
//
#include "sender.hpp"

extern Configure config;

void Sender::run() {

    Chunk tmpChunk;

    while (1) {
        this->extractMQ(tmpChunk);
        this->sendData(tmpChunk);

    }
}

Sender::Sender() {
    Socket.init(CLIENTTCP,config.getStorageServerIP(),config.getStorageServerPort());
}

Sender::~Sender() {
    Socket.finish();
}



bool Sender::sendData(Chunk &tmpChunk) {
    string data=serialize(tmpChunk);
    Socket.Send(data);
}