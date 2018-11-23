#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace std;

#define MAXLINE 5
#define OPEN_MAX 100
#define LISTENQ 20
#define SERV_PORT 5000
#define INFTIM 1000

void setnonblocking(int sock){
  int opts;
  opts=fcntl(sock,F_GETFL);
  if(opts<0){
    perror("fcntl(sock,GETFL)");
    return;
  }
  opts = opts|O_NONBLOCK;
  if(fcntl(sock,F_SETFL,opts)<0){
    perror("fcntl(sock,SETFL,opts)");
    return;
  }
}

int main(){
  int i,listenfd, connfd, sockfd,epfd,nfds, portnumber;
  char line[MAXLINE];
  socklen_t clilen;
  portnumber = 5000;
  struct epoll_event ev,events[20];
  epfd=epoll_create(256);
  struct sockaddr_in clientaddr;
  struct sockaddr_in serveraddr;
  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port=htons(portnumber);

  bind(listenfd,(sockaddr *)&serveraddr, sizeof(serveraddr));
  listen(listenfd, LISTENQ);
  ev.data.fd=listenfd;
  ev.events=EPOLLIN|EPOLLET;
  epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);


  while(1){
    nfds=epoll_wait(epfd,events,20,-1);
    cout << "\nepoll_wait returns\n";

    for(i=0;i<nfds;++i){
      if(events[i].data.fd==listenfd){
        connfd = accept(listenfd,(sockaddr *)&clientaddr, &clilen);
        if(connfd<0){
          perror("connfd<0");
          return (1);
        }
        char *str = inet_ntoa(clientaddr.sin_addr);
        cout << "accapt a connection from " << str << endl;

        setnonblocking(connfd);
        ev.data.fd=connfd;

        ev.events=EPOLLIN | EPOLLET;
        epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
      }
      else if(events[i].events & EPOLLIN){
        cout << "EPOLLIN" << endl;
        if ((sockfd = events[i].data.fd) < 0)
          continue;

        char * head = line;

        recv(sockfd, head , MAXLINE, 0);
        cout << "we have read from the client : " << line;
        ev.data.fd=sockfd;
        ev.events = EPOLLOUT | EPOLLET;

        epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
      }
      else if(events[i].events & EPOLLOUT){
        const char str[] = "hello from epoll : this is a long string which may be cut by the net\n";
        memcpy(line, str, sizeof(char)*5);
        cout << "Write " << line << endl;
        sockfd = events[i].data.fd;
        int count = 0;
        char * head = line;

        send(sockfd, head + count, MAXLINE, 0);

        ev.data.fd=sockfd;
        ev.events=EPOLLIN | EPOLLET;
        epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
      }
    }
  }
  return 0;
}