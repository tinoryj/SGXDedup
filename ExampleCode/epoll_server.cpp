#include <sys/epoll.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace std;

struct self{
    int fd;
    char buffer[64];
    self(int x):fd(x){};
}*p;

void setnonblock(int x){
    int flag=fcntl(x,F_GETFL,0);
    fcntl(x,F_SETFL,flag|O_NONBLOCK);
}

int main(){
    int listenfd,epfd,nfds,confd;
    struct epoll_event tev,event[30];

    struct sockaddr_in serveraddr;

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_port=htons(6667);
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
                setnonblock(confd);
                p=new self(confd);
                tev.data.ptr=(void*)p;
                tev.events=EPOLLET|EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,confd,&tev);
                continue;
            }
            if(event[i].events&EPOLLIN){
                int tfd=q->fd;
                p=new self(tfd);
                int len=read(tfd,p->buffer,sizeof(p->buffer));
                if(len<=0){
                    cout<<"close\n";
                    epoll_ctl(epfd,EPOLL_CTL_DEL,tfd,&event[i]);
                    close(tfd);
                    continue;
                }
                cout<<p->buffer<<endl;
                if(event[i].events&EPOLLOUT) goto send;
                tev=event[i];
                tev.data.ptr=(void*)p;
                tev.events=EPOLLOUT|EPOLLET|EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_MOD,tfd,&tev);
            }
            if(event[i].events&EPOLLOUT){
                send:
                int tfd=q->fd;
                p=(self*)event[i].data.ptr;
    //            p->buffer[0]='O';
//              p->buffer[1]='\0';
                int len=write(tfd,p->buffer,3);
                cout<<len<<endl;
                if(len<=0){
                    cout<<"-1close\n";
                    epoll_ctl(epfd,EPOLL_CTL_DEL,tfd,&event[i]);
                    close(tfd);
                    continue;
                }
                tev=event[i];
                tev.events=EPOLLIN|EPOLLET|EPOLLOUT;
                epoll_ctl(epfd,EPOLL_CTL_MOD,tfd,&tev);
            }
        }
    }
    return 0;
}