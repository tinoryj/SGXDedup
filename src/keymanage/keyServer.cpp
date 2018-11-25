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
    PEM_read_bio_RSAPrivateKey(_key,&_rsa,NULL,NULL);

    _bnCTX=BN_CTX_new();
}

keyServer::~keyServer(){
    BIO_free_all(_key);
    RSA_free(_rsa);
}

void keyServer::runRecv(){
    _messageQueue mq("epoll to workload",WRITE_MESSAGE);
    epoll_event ev,event[100];
    int epfd,fd;
    epfd=epoll_create(20);
    message* msg=new message();
    //msg->con.first=_keySecurityChannel->listenFd;
    msg->con.fd=_keySecurityChannel->listenFd;
    msg->epfd=epfd;
    ev.data.ptr=(void*)msg;
    epoll_ctl(epfd,EPOLL_CTL_ADD,_keySecurityChannel->listenFd,&ev);

    while(1){
        int nfd=epoll_wait(epfd,event,20,-1);
        for(int i=0;i<nfd;i++){

            //msg 存在耦合，在数据发送后将 msg 删除
            msg=(message*)event[i].data.ptr;
            if(msg->con.fd==_keySecurityChannel->listenFd){
            //if(msg->con.first==_keySecurityChannel->listenFd){

                message* msg1=new message();
                *msg1=*msg;
                msg->con=_keySecurityChannel->sslListen();
#ifdef DEBUG
                std::cout<<"accept an connection\n";
#endif
                ev.data.ptr=(message*)msg1;
                ev.events=EPOLLET|EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,msg1->con.fd,&ev);
                //epoll_ctl(epfd,EPOLL_CTL_ADD,msg1->con.first,&ev);
            }
            else if(event[i].events& EPOLLIN){
                string hash;
                //_keySecurityChannel->sslRead(msg->con.second,hash);
                _keySecurityChannel->sslRead(msg->con.sslsocket,hash);
                memcpy(msg->hash,hash.c_str(),sizeof(msg->hash));
                mq.push(*msg);
#ifdef DEBUG
                std::cout<<"Receive an message\n";
#endif
            }
            else if(event[i].events& EPOLLOUT){
#ifdef DEBUG
                std::cout<<"Send back message\n";
#endif
                _keySecurityChannel->sslWrite(msg->con.sslsocket,msg->key);
                //_keySecurityChannel->sslWrite(msg->con.second,msg->key);

                //?
                ev.events=EPOLLET|EPOLLIN;
                //epoll_ctl(epfd,EPOLL_CTL_MOD,msg->con.first,&ev);
                epoll_ctl(epfd,EPOLL_CTL_MOD,msg->con.fd,&ev);
            }
        }
    }
    /*
    for(auto it:thList){
        it->join();
    }
    */
}

void keyServer::runKeyGen(){
    std::string key;
    epoll_event ev;
    _messageQueue mq("epoll to workload",READ_MESSAGE);
    message* msg = new message();
    while(1){
        mq.pop(*msg);
#ifdef DEBUG
        std::cout<<"start keygen\n";
#endif
        this->workloadProgress(msg->hash,key);
        memcpy(msg->key,key.c_str(),sizeof(msg->key));
        ev.data.ptr=(message*)msg;
        ev.events=EPOLLET|EPOLLOUT;
        epoll_ctl(msg->epfd,EPOLL_CTL_MOD,msg->con.fd,&ev);
        //epoll_ctl(msg->epfd,EPOLL_CTL_MOD,msg->con.first,&ev);
    }
}

bool keyServer::keyGen(std::string hash,std::string& key){
    if(kCache.existsKeyinCache(hash)){
        key=kCache.getKeyFromCache(hash);
        return true;
    }

    BIGNUM* result;
    char buffer[128];
    memset(buffer,0,sizeof(buffer));

    BN_bin2bn((const unsigned char*)hash.c_str(),128,result);

    //result=hash^d
    BN_mod_exp(result,result,_rsa->d,_rsa->n,_bnCTX);
    BN_bn2bin(result,(unsigned char*)buffer+(128-BN_num_bytes(result)));
    key=buffer;

    kCache.insertKeyToCache(hash,key);
}