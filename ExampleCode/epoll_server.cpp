#include <sys/epoll.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

struct self{
    int fd;
    char buffer[64];
    self(int x):fd(x){};
}*p;

void setblock(int x){

}

int main(){
    int listenfd,epfd,nfds,confd;
    struct epoll_event tev,event[30];

    struct sockaddr_in serveraddr;

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_port=htons(6666);
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);


    listenfd=socket(AF_INET,SOCK_STREAM,0);
    bind(listenfd,(sockaddr*)&serveraddr,sizeof(serveraddr));
    listen(listenfd,10);

    epfd=epoll_create(10);
    tev.data.fd=listenfd;
    tev.events=EPOLLET|EPOLLIN;
    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&tev);

    while(1){
        nfds=epoll_wait(epfd,event,20,-1);
        for(int i=0;i<nfds;i++){
            self* q=(self*)event[i].data.ptr;
            if(event[i].data.fd==listenfd){
                confd=accept(listenfd,(sockaddr*)NULL,NULL);
                setblock(confd);
                p=new self(confd);
                tev.data.ptr=(void*)p;
                tev.events=EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_ADD,confd,&tev);
            }
            else if(event[i].events&EPOLLIN){
                int tfd=q->fd;
                p=new self(tfd);
                int len=read(tfd,p->buffer,sizeof(p->buffer));
                if(len==0){
                    cout<<"close\n";
                    epoll_ctl(epfd,EPOLL_CTL_DEL,tfd,&event[i]);
                    continue;
                }
                cout<<p->buffer<<endl;
                tev=event[i];
                tev.data.ptr=(void*)p;
                tev.events=EPOLLOUT|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,tfd,&tev);
            }
            else if(event[i].events&EPOLLOUT){
                int tfd=q->fd;
                p=(self*)event[i].data.ptr;
                write(tfd,p->buffer,sizeof(p->buffer));
                tev=event[i];
                tev.events=EPOLLIN|EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,tfd,&tev);
            }
        }
    }
    return 0;
}