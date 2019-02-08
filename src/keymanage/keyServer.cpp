//
// Created by a on 11/17/18.
//

#include <sys/epoll.h>
#include "keyServer.hpp"
extern Configure config;
extern util::keyCache kCache;

keyServer::keyServer(){

    _keySecurityChannel=new ssl(config.getKeyServerIP(),config.getKeyServerPort(),SERVERSIDE);
    _rsa=RSA_new();
    _key=NULL;
    _key=BIO_new_file(KEYMANGER_PRIVATE_KEY,"r");

    char passwd[5]="1111";passwd[4]='\0';
    PEM_read_bio_RSAPrivateKey(_key,&_rsa,NULL,passwd);

    _bnCTX=BN_CTX_new();
}

keyServer::~keyServer(){
    BIO_free_all(_key);
    RSA_free(_rsa);
}

void keyServer::runRecv(){
    map<int,SSL*>sslconnection;
    _messageQueue mq("epoll to workload",WRITE_MESSAGE);
    epoll_event ev,event[100];
    int epfd,fd;
    message* msg=new message();

    epfd=epoll_create(20);
    msg->fd=_keySecurityChannel->listenFd;
    msg->epfd=epfd;
    ev.data.ptr=(void*)msg;
    ev.events=EPOLLET|EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,_keySecurityChannel->listenFd,&ev);

    while(1){
        int nfd=epoll_wait(epfd,event,20,-1);
        for(int i=0;i<nfd;i++){

            //msg 存在耦合，在数据发送后将 msg 删除
            msg=(message*)event[i].data.ptr;
            if(msg->fd==_keySecurityChannel->listenFd){

                message* msg1=new message();
                std::pair<int,SSL*> con=_keySecurityChannel->sslListen();
                *msg1=*msg;
                msg1->fd=con.first;
                sslconnection[con.first]=con.second;
#ifdef DEBUG
                std::cout<<"accept an connection\n";
#endif
                ev.data.ptr=(void*)msg1;
                ev.events=EPOLLET|EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,msg1->fd,&ev);
                continue;
            }
            if(event[i].events& EPOLLIN){
                string hash;
                _keySecurityChannel->sslRead(sslconnection[msg->fd],hash);
                memcpy(msg->hash,hash.c_str(),sizeof(msg->hash));
                mq.push(*msg);
                delete msg;
#ifdef DEBUG
                std::cout<<"Receive an message\n";
#endif
                continue;
            }
            if(event[i].events& EPOLLOUT){
#ifdef DEBUG
                std::cout<<"Send back message\n";
#endif
                _keySecurityChannel->sslWrite(sslconnection[msg->fd],msg->key);

                ev.events=EPOLLET|EPOLLIN;
                ev.data.ptr=(void*)msg;
                epoll_ctl(epfd,EPOLL_CTL_MOD,msg->fd,&ev);
                continue;
            }
        }
    }
}

void keyServer::runKeyGen(){
    std::string key;
    epoll_event ev;
    ev.events=EPOLLET|EPOLLOUT;
    _messageQueue mq("epoll to workload",READ_MESSAGE);
    message* msg = new message();
    while(1){
        if(!mq.pop(*msg)){
            continue;
        }
#ifdef DEBUG
        std::cout<<"start keygen\n";
#endif
        this->workloadProgress(msg->hash,key);
        memcpy(msg->key,key.c_str(),sizeof(msg->key));
        ev.data.ptr=(void*)msg;
        epoll_ctl(msg->epfd,EPOLL_CTL_MOD,msg->fd,&ev);
    }
}

bool keyServer::keyGen(std::string hash,std::string& key){
    if(kCache.existsKeyinCache(hash)){
        key=kCache.getKeyFromCache(hash);
        return true;
    }

    BIGNUM* result=BN_new();
    char buffer[128];
    memset(buffer,0,sizeof(buffer));

    BN_bin2bn((const unsigned char*)hash.c_str(),128,result);

    //result=hash^d
    BN_mod_exp(result,result,_keyD,_keyN,_bnCTX);
    BN_bn2bin(result,(unsigned char*)buffer+(128-BN_num_bytes(result)));
    key=buffer;

    kCache.insertKeyToCache(hash,key);
}