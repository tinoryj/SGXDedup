#include <string>
#include <cstring>
#include <sys/socket.h>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
int main(){
    string serverip="127.0.0.1";
    int serverfd;
    char buffer[64],bufferin[64];
    struct sockaddr_in serveraddr;

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(6667);
    inet_pton(AF_INET,serverip.c_str(),&serveraddr.sin_addr);

    serverfd=socket(AF_INET,SOCK_STREAM,0);

    connect(serverfd,(sockaddr*)&serveraddr,sizeof(serveraddr));
    cin>>buffer;
    int i=0;
    while(1){
        cout<<"write : "<<buffer<<endl;
        if(i==2)break;
        i++;
        write(serverfd,buffer,sizeof(buffer));
        write(serverfd,buffer,sizeof(buffer));
        cout<<read(serverfd,buffer, sizeof(bufferin))<<".";
        cout<<"read :  "<<buffer<<endl;
    }
    return 0;
}