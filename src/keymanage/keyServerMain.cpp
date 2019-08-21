#include "keyServer.hpp"
#include "socket.hpp"

Configure config("config.json");

int main()
{
    Socket socket(SERVER_TCP, "", config.getKeyServerPort());
    boost::thread* th;
    Socket tmpServerSocket = socket.Listen();
    keyServer server(tmpServerSocket);
    while (true) {
        Socket tmpSocket = socket.Listen();
        th = new boost::thread(boost::bind(&keyServer::run, &server, tmpSocket));
        th->detach();
    }
    return 0;
}