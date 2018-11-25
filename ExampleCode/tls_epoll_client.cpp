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

#define CLCERT "key/clientcert.pem"
#define CLKEY  "key/client.key"
#define CACERT "key/cacert.pem"
#define PORT 6666
#define IP "127.0.0.1"

using namespace std;

int serverfd,epfd;

struct message{
    char buffer[1024];
    int fd;
    message(int x):fd(x){}
}*p;

void readthread(){
    struct epoll_event ev;
    message *q;
    while(1){
        q=new message(serverfd);
        cin>>q->buffer;
        ev.events=EPOLLOUT|EPOLLET;
        ev.data.ptr=(void*)q;
        epoll_ctl(epfd,EPOLL_CTL_MOD,serverfd,&ev);
    }
}

void setnonblock(){
    fcntl(serverfd,F_SETFL,fcntl(serverfd,F_GETFD,0)|O_NONBLOCK);
}

int main(){
    struct sockaddr_in serveraddr;
    SSL_CTX* ctx;
    SSL *ssl;
    struct epoll_event ev,event[20];

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ctx=SSL_CTX_new(TLSv1_1_client_method());
    SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER,NULL);
    SSL_CTX_load_verify_locations(ctx,CACERT,NULL);
    SSL_CTX_use_certificate_file(ctx,CLCERT,SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx,CLKEY,SSL_FILETYPE_PEM);
    if(!SSL_CTX_check_private_key(ctx)){
        cerr<<"1\n";
        return 0;
    }

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(PORT);
    serveraddr.sin_addr.s_addr=inet_addr(IP);

    serverfd=socket(AF_INET,SOCK_STREAM,0);
   // setnonblock();
    int x=connect(serverfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
    cout<<x<<endl;
    ssl=SSL_new(ctx);
    SSL_set_fd(ssl,serverfd);
    int y=SSL_connect(ssl);
    cout<<y<<endl;

    epfd=epoll_create(20);

    p=new message(serverfd);
    ev.data.ptr=(message*)p;
    ev.events=EPOLLET;
    epoll_ctl(epfd,EPOLL_CTL_ADD,serverfd,&ev);

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
}
