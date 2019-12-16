#include "ssl.hpp"

ssl::ssl(std::string ip, int port, int scSwitch)
{
    this->serverIP_ = ip;
    this->port_ = port;
    this->listenFd_ = socket(AF_INET, SOCK_STREAM, 0);

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    memset(&sockAddr_, 0, sizeof(sockAddr_));
    std::string keyFile, crtFile;

    sockAddr_.sin_port = htons(port);
    sockAddr_.sin_family = AF_INET;

    //char passwd[5] = "1111";

    switch (scSwitch) {
    case SERVERSIDE: {
        ctx_ = SSL_CTX_new(TLS_server_method());
        SSL_CTX_set_mode(ctx_, SSL_MODE_AUTO_RETRY);
        crtFile = SERVER_CERT;
        keyFile = SERVER_KEY;
        sockAddr_.sin_addr.s_addr = htons(INADDR_ANY);
        if (bind(listenFd_, (sockaddr*)&sockAddr_, sizeof(sockAddr_)) == -1) {
            std::cerr << "Can not bind to sockfd" << endl;
            std::cerr << "May cause by shutdown server before client" << endl;
            std::cerr << "Wait for 30 sec and try again" << endl;
            exit(1);
        }
        if (listen(listenFd_, 10) == -1) {
            std::cerr << "Can not set listen socket" << endl;
            exit(1);
        }
        break;
    }
    case CLIENTSIDE: {
        ctx_ = SSL_CTX_new(TLS_client_method());
        keyFile = CLIENT_KEY;
        crtFile = CLIENT_CERT;
        sockAddr_.sin_addr.s_addr = inet_addr(ip.c_str());
        break;
    };
    }

    SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, NULL);
    if (!SSL_CTX_load_verify_locations(ctx_, CA_CERT, NULL)) {
        std::cerr << "Wrong CA crt file at ssl.cpp:ssl(ip,port)" << endl;
        exit(1);
    }
    if (!SSL_CTX_use_certificate_file(ctx_, crtFile.c_str(), SSL_FILETYPE_PEM)) {
        std::cerr << "Wrong crt file at ssl.cpp:ssl(ip,port)" << endl;
        exit(1);
    }
    if (!SSL_CTX_use_PrivateKey_file(ctx_, keyFile.c_str(), SSL_FILETYPE_PEM)) {
        std::cerr << "Wrong key file at ssl.cpp:ssl(ip,port)" << endl;
        exit(1);
    }
    if (!SSL_CTX_check_private_key(ctx_)) {
        std::cerr << "1" << endl;
        exit(1);
    }
}

ssl::~ssl()
{
    /*for(auto fd:_fdList){
        close(fd);
    }
    for(auto sslConection:_sslList){
        SSL_shutdown(sslConection);
        SSL_free(sslConection);
    }*/
}
std::pair<int, SSL*> ssl::sslConnect()
{
    //std::pair<int,SSL*> ssl::sslConnect(){
    int fd;
    SSL* sslConection;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr*)&sockAddr_, sizeof(sockaddr)) < 0) {
        std::cerr << "ERROR Occur on ssl(fd) connect" << endl;
        exit(1);
    }
    sslConection = SSL_new(ctx_);
    SSL_set_fd(sslConection, fd);
    SSL_connect(sslConection);

    //_fdList.push_back(fd);
    //_sslList.push_back(sslConection);
    return std::make_pair(fd, sslConection);
}

std::pair<int, SSL*> ssl::sslListen()
{
    //std::pair<int,SSL*> ssl::sslListen(){
    int fd;
    fd = accept(listenFd_, (struct sockaddr*)NULL, NULL);
    SSL* sslConection = SSL_new(ctx_);
    SSL_set_fd(sslConection, fd);
    SSL_accept(sslConection);

    //_fdList.push_back(fd);
    //_sslList.push_back(sslConection);
    return std::make_pair(fd, sslConection);
}

bool ssl::sslRead(SSL* connection, std::string& data)
{
    int recvd = 0, len = 0;
    if (SSL_read(connection, (char*)&len, sizeof(int)) == 0) {
        return false;
    }
    data.resize(len);
    while (recvd < len) {
        recvd += SSL_read(connection, &data[recvd], 4096);
    }
    data.resize(len);
    return true;
}

void ssl::sslWrite(SSL* connection, std::string data)
{
    int len = data.length();
    SSL_write(connection, (char*)&len, sizeof(int));
    SSL_write(connection, data.c_str(), sizeof(char) * data.length());
}