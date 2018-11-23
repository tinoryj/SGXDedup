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


int main(int argc, char **argv)
{
    int sockfd;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(5000);   
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    connect(sockfd, (sockaddr *) &servaddr, sizeof(servaddr)) ;

    char input[100];
    while (cin>>input)
    {
        write(sockfd, input, strlen(input));

        int n = 0;
        int count = 0;
        while (1)
        {
            n = read(sockfd, recvline + count, MAXLINE);
            if (n == MAXLINE)
            {
                count += n;
                continue;
            }
            else 
                break;
        }
        printf("%s\n", recvline);
    }
    exit(0);
}