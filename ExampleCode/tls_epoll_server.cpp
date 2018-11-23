//
// Created by a on 11/20/18.
//

#include <bits/stdc++.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <boost/thread.hpp>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define CACERT "key/cacert.crt"
#define SECERT "key/server.crt"
#define SEKEY  "key/server.key"

#define PORT 6666
#define IP "127.0.0.1"

using namespace std;

int serverfd,epfd,tfd;

struct message{
    char buffer[1024];
    int fd;
    message(int x):fd(x){}
}*p;

void readthread(){
    struct epoll_event ev;
    message *q;
    while(1){
        q=new message(tfd);
        cin>>q->buffer;
        ev.events=EPOLLOUT|EPOLLET;
        ev.data.ptr=(void*)q;
        epoll_ctl(epfd,EPOLL_CTL_MOD,tfd,&ev);
    }
}

void setnonblock(){
    fcntl(tfd,F_SETFL,fcntl(tfd,F_GETFD,0)|O_NONBLOCK);
}

int main(){
    struct sockaddr_in serveraddr;
    SSL_CTX* ctx;
    SSL *ssl;
    struct epoll_event ev,event[20];
    string serverip=IP;

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(PORT);
    serveraddr.sin_addr.s_addr=htons(INADDR_ANY);

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ctx=SSL_CTX_new(TLSv1_1_server_method());
    SSL_CTX_set_mode(ctx,SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);
    SSL_CTX_load_verify_locations(ctx,CACERT,NULL);
    SSL_CTX_use_certificate_file(ctx,SECERT,SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx,SEKEY,SSL_FILETYPE_PEM);
    if(!SSL_CTX_check_private_key(ctx)){
        cerr<<"1\n";
        return 0;
    }

    serverfd=socket(AF_INET,SOCK_STREAM,0);
    bind(serverfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
    setnonblock();
    listen(serverfd,10);
    tfd=accept(serverfd,(struct sockaddr*)NULL,NULL);

    ssl=SSL_new(ctx);
    SSL_set_fd(ssl,tfd);
    SSL_accept(ssl);

    epfd=epoll_create(20);

    p=new message(tfd);
    p->buffer[0]=p->buffer[1]='a';
    ev.data.ptr=(message*)p;
    ev.events=EPOLLET|EPOLLOUT;
    epoll_ctl(epfd,EPOLL_CTL_ADD,tfd,&ev);

    boost::thread th(readthread);

    while(1){
        int nfd=epoll_wait(epfd,event,10,-1);
        for(int i=0;i<nfd;i++){
            if(event[i].events&EPOLLIN){
                char buffer[1024];
                SSL_read(ssl,buffer,sizeof(buffer));
                cout<<buffer<<endl;
            }
            if(event[i].events&EPOLLOUT){
                p=(message*)event[i].data.ptr;
                SSL_write(ssl,p->buffer,sizeof(p->buffer));
                delete p;
            }
        }
    }

    th.join();
}
