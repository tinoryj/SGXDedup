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
    char buffer[64];
    struct sockaddr_in serveraddr;

    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(6666);
    inet_pton(AF_INET,serverip.c_str(),&serveraddr.sin_addr);

    serverfd=socket(AF_INET,SOCK_STREAM,0);

    connect(serverfd,(sockaddr*)&serveraddr,sizeof(serveraddr));

    while(1){
        cin>>buffer;
        write(serverfd,buffer,sizeof(buffer));
        read(serverfd,buffer, sizeof(buffer));
        cout<<buffer<<endl;
    }
    return 0;
}