#ifndef KETMANGER_HPP
#define KETMANGER_HPP

#include "_keyManager.hpp"
#include "chunk.hpp"
#include "ssl.hpp"
#include <string>
#include <boost/lockfree/queue.hpp>

#define SERVERSIDE 0
#define CLIENTSIDE 1

extern configure config;

class keyManager::public _KeyManager{
private:
	boost::lockfree::queue<SSL*>_freeKeyConnection;
	ssl *_keyConnection;
	std::stringstream serializeBufferIn
	boost::archive::text_iarchive serializeIn(serializeBufferIn);

public:
	void keyManger(int scSwitch);
	void keyExchange(Chunk champion,string& newKey){
		while(1){
			if(_freeConnection.empty())continue;
			SSL* ssl=_freeConnection.pop();
			keyExchangeClient(ssl,champion,newKey);
			_freeConnection.push(ssl);
		}
	}
	void clientThreadHandle(SSL* connectedFd,Chunk champion,string newKey);
	void serverThreadHandle(SSL* connectedFd);
};

keyManager::keyManager(int scSwitch){
	_keyConnection=new ssl(config.getkeyServerIP(),config.getkeyServerPort,scSwitch);
	switch(scSwitch){
		case SERVERSIDE:{
			_freeConnection.push(_keyConnection->listen());
			boost::function<void(SSL*)>fun=boost::bind(serverThreadHandle,_1);
			while(SSL* connectedFd=_keyConnection->listen()){
				fun(connectedFd);
			}
			break;
		}
		case CLIENTSIDE:{
			for(int i=0;i<config.getMaxThreadLimits();i++){
				_freeConnection.push(_keyConnection->connect());
			}
			break;
		}
	}
}

void KeyManger::clientThreadHandle(SSL* connectedFd,Chunk champion,string newKey){
		string writeBuffer;
		char readBuffer[4096];
		serializeIn<<champion;
		serializeBufferIn>>writeBuffer;
		SSL_write(connectedFd,writeBuffer.c_str(),sizeof(char)*writeBuffer.length());
		SSL_read(connectedFd,readBuffer,sizeof(readBuffer));
		newKey=readBuffer;
}

void keyManger::serverThreadHandle(SSL* connectedFd){
	char readBuffer[4096];
	while(1){
		SSL_read(connectedFd,readBuffer,sizeof(readBuffer));

		readBuffer[0]+255;
	
		SSL_write(connectedFd,readBuffer,sizeof(readBuffer));
	}
}

#endif