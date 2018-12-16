//
// Created by a on 11/17/18.
//


#include "ssl.hpp"


ssl::ssl(std::string ip,int port,int scSwitch){
    this->_serverIP=ip;
    this->_port=port;

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    memset(&_sockAddr,0,sizeof(_sockAddr));
    std::string keyFile,crtFile;

    _sockAddr.sin_port=htons(port);
    _sockAddr.sin_family=AF_INET;

    switch(scSwitch){
        case SERVERSIDE:{
            _ctx=SSL_CTX_new(TLSv1_server_method());
            SSL_CTX_set_mode(_ctx,SSL_MODE_AUTO_RETRY);
            crtFile=SECRT;
            keyFile=SEKEY;
            _sockAddr.sin_addr.s_addr=htons(INADDR_ANY);
            bind(listenFd,(sockaddr*)&_sockAddr,sizeof(_sockAddr));
            listen(listenFd,10);
            break;
        }
        case CLIENTSIDE:{
            _ctx=SSL_CTX_new(TLSv1_client_method());
            keyFile=CLKEY;
            crtFile=CLCRT;
            _sockAddr.sin_addr.s_addr=inet_addr(ip.c_str());
            break;
        };
    }


    SSL_CTX_set_verify(_ctx,SSL_VERIFY_PEER,NULL);
    if(!SSL_CTX_load_verify_locations(_ctx,CACRT,NULL)){
        std::cerr<<"Wrong CA crt file at ssl.cpp:ssl(ip,port)\n";
        exit(1);
    }
    if(!SSL_CTX_use_certificate_file(_ctx,crtFile.c_str(),SSL_FILETYPE_PEM)){
        std::cerr<<"Wrong crt file at ssl.cpp:ssl(ip,port)\n";
        exit(1);
    }
    if(!SSL_CTX_use_PrivateKey_file(_ctx,keyFile.c_str(),SSL_FILETYPE_PEM)){
        std::cerr<<"Wrong key file at ssl.cpp:ssl(ip,port)\n";
        exit(1);
    }
    if(!SSL_CTX_check_private_key(_ctx)){
        std::cerr<<"1\n";
        exit(1);
    }
}

ssl::~ssl(){
    /*for(auto fd:_fdList){
        close(fd);
    }
    for(auto sslConection:_sslList){
        SSL_shutdown(sslConection);
        SSL_free(sslConection);
    }*/
}
std::pair<int,SSL*> ssl::sslConnect() {
//std::pair<int,SSL*> ssl::sslConnect(){
    int fd;
    SSL* sslConection;

    fd=socket(AF_INET,SOCK_STREAM,0);
    if(connect(fd,(struct sockaddr*)&_sockAddr,sizeof(sockaddr))<0){
        std::cerr<<"ERROR Occur on ssl(fd) connect\n";
        exit(1);
    }
    sslConection=SSL_new(_ctx);
    SSL_set_fd(sslConection,fd);
    SSL_connect(sslConection);


    //_fdList.push_back(fd);
    //_sslList.push_back(sslConection);
    return std::make_pair(fd,sslConection);
}

std::pair<int,SSL*> ssl::sslListen() {
//std::pair<int,SSL*> ssl::sslListen(){
    int fd;
    fd=accept(listenFd,(struct sockaddr*)NULL,NULL);
    SSL* sslConection=SSL_new(_ctx);
    SSL_set_fd(sslConection,fd);
    SSL_accept(sslConection);

    //_fdList.push_back(fd);
    //_sslList.push_back(sslConection);
    return std::make_pair(fd,sslConection);
}

void ssl::sslRead(SSL* connection,std::string& data){
    char buffer[4096];
    SSL_read(connection,buffer,sizeof(buffer));
    data=buffer;
}

void ssl::sslWrite(SSL* connection,std::string data){
    SSL_write(connection,data.c_str(),sizeof(char)*data.length());
}