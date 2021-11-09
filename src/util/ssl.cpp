#include "ssl.hpp"
#include <fcntl.h>

bool IsFileUsed(const char* filePath)
{
    bool ret = false;
    if ((access(filePath, 2)) == -1) {
        ret = true;
    }
    return ret;
}

ssl::ssl(string ip, int port, int scSwitch)
{
    this->_serverIP = ip;
    this->_port = port;

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
#if SYSTEM_DEBUG_FLAG == 1
    while (true) {
        if (!IsFileUsed(CACRT) && !IsFileUsed(CLCRT) && !IsFileUsed(CLKEY) && !IsFileUsed(SECRT) && !IsFileUsed(SEKEY)) {
            cout << "SSL : Key files not in used, Start ssl connection" << endl;
            break;
        } else {
            cout << "SSL : Key files status " << IsFileUsed(CACRT) << IsFileUsed(CLCRT) << IsFileUsed(CLKEY) << IsFileUsed(SECRT) << IsFileUsed(SEKEY) << endl;
        }
    }
#endif
    switch (scSwitch) {
    case SERVERSIDE: {
        _ctx = SSL_CTX_new(TLS_server_method());
        // _ctx = SSL_CTX_new(SSLv23_server_method());
        if (_ctx == NULL) {
            cerr << "SSL : create ssl _ctx error:" << endl;
            ERR_print_errors_fp(stderr);
            exit(1);
        }
        SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

        if (SSL_CTX_load_verify_locations(_ctx, CACRT, NULL) <= 0) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        if (SSL_CTX_use_certificate_file(_ctx, SECRT, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        if (SSL_CTX_use_PrivateKey_file(_ctx, SEKEY, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        if (!SSL_CTX_check_private_key(_ctx)) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        this->listenFd = socket(PF_INET, SOCK_STREAM, 0);
        if (this->listenFd == -1) {
            cerr << "SSL : init socket error" << endl;
            exit(1);
        }
        bzero(&_sockAddr, sizeof(_sockAddr));
        _sockAddr.sin_family = PF_INET;
        _sockAddr.sin_port = htons(port);
        _sockAddr.sin_addr.s_addr = htons(INADDR_ANY);

        if (bind(this->listenFd, (struct sockaddr*)&_sockAddr, sizeof(struct sockaddr)) == -1) {
            cerr << "SSL : Can not bind to sockfd" << endl
                 << "\tMay cause by shutdown server before client" << endl
                 << "\tWait for 1 min and try again" << endl;
            exit(1);
        }
        if (listen(listenFd, 10) == -1) {
            cerr << "SSL : Can not set listen socket" << endl;
            exit(1);
        }
        break;
    }
    case CLIENTSIDE: {
        _ctx = SSL_CTX_new(TLS_client_method());
        // _ctx = SSL_CTX_new(SSLv23_client_method());
        if (_ctx == NULL) {
            cerr << "SSL : create ssl _ctx error:" << endl;
            ERR_print_errors_fp(stderr);
            exit(1);
        }
        SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

        if (SSL_CTX_load_verify_locations(_ctx, CACRT, NULL) <= 0) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        if (SSL_CTX_use_certificate_file(_ctx, CLCRT, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        if (SSL_CTX_use_PrivateKey_file(_ctx, CLKEY, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        if (!SSL_CTX_check_private_key(_ctx)) {
            ERR_print_errors_fp(stdout);
            exit(1);
        }
        // this->listenFd = socket(AF_INET, SOCK_STREAM, 0);
        // if (this->listenFd < 0) {
        //     cerr << "SSL : init socket error" << endl;
        //     exit(1);
        // }
        bzero(&_sockAddr, sizeof(_sockAddr));
        _sockAddr.sin_family = AF_INET;
        _sockAddr.sin_port = htons(port);
        if (inet_aton(ip.c_str(), (struct in_addr*)&_sockAddr.sin_addr.s_addr) == 0) {
            cerr << "SSL : create client s_addr error" << endl;
        }
        break;
    };
    }
}

ssl::~ssl()
{
    SSL_CTX_free(_ctx);
}

pair<int, SSL*> ssl::sslConnect()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        cerr << "SSL : init socket error" << endl;
        exit(1);
    }
    SSL* sslConection;

    if (connect(fd, (struct sockaddr*)&_sockAddr, sizeof(_sockAddr)) < 0) {
        cerr << "SSL : ERROR Occur on ssl(fd) connect" << endl;
        exit(1);
    }

    sslConection = SSL_new(_ctx);
    int ret = SSL_set_fd(sslConection, fd);
    if (ret != 1) {
        cerr << "SSL : error set connected fd" << endl;
        exit(1);
    } else {
        ret = SSL_connect(sslConection);
        if (ret == -1) {
            cerr << "SSL : error set connection, status = " << ret << endl;
            ERR_print_errors_fp(stderr);
            exit(1);
        }
    }
    X509* cert = SSL_get_peer_certificate(sslConection);
    if (SSL_get_verify_result(sslConection) != X509_V_OK) {
        cerr << "SSL : X509 crt error" << endl;
        exit(1);
    }
    X509_free(cert);
    return make_pair(fd, sslConection);
}

pair<int, SSL*> ssl::sslListen()
{
    struct sockaddr_in their_addr;
    socklen_t len = sizeof(struct sockaddr);
    int fd = accept(listenFd, (struct sockaddr*)&their_addr, &len);
    if (fd == -1) {
        cerr << "SSL : error accept connection" << endl;
        exit(1);
    }
    SSL* sslConection = SSL_new(_ctx);
    SSL_set_fd(sslConection, fd);
    // SSL_set_accept_state(sslConection);
    if (SSL_accept(sslConection) == -1) {
        cerr << "SSL : error accept ssl connection" << endl;
        close(fd);
        exit(1);
    }
    X509* cert = SSL_get_peer_certificate(sslConection);
    if (SSL_get_verify_result(sslConection) != X509_V_OK) {
        cerr << "SSL : X509 crt error" << endl;
        exit(1);
    }
    return make_pair(fd, sslConection);
}

bool ssl::recv(SSL* connection, char* data, int& dataSize)
{
    int recvd = 0, len = 0;
    int ret = SSL_read(connection, (char*)&len, sizeof(int));
    if (ret <= 0) {
        getSSLErrorMessage(connection, ret);
        return false;
    }
    while (recvd < len) {
        ret = SSL_read(connection, data + recvd, len - recvd);
        if (ret <= 0) {
            getSSLErrorMessage(connection, ret);
            return false;
        }
        recvd += ret;
    }
    dataSize = len;
    return true;
}

bool ssl::send(SSL* connection, char* data, int dataSize)
{
    int ret = SSL_write(connection, (char*)&dataSize, sizeof(int));
    if (ret == 0) {
        getSSLErrorMessage(connection, ret);
        return false;
    }
    int sendSize = 0;
    while (sendSize < dataSize) {
        ret = SSL_write(connection, data + sendSize, dataSize - sendSize);
        if (ret <= 0) {
            getSSLErrorMessage(connection, ret);
            return false;
        }
        sendSize += ret;
    }
    return true;
}

bool ssl::getSSLErrorMessage(SSL* connection, int ret)
{
    int errorStatus = SSL_get_error(connection, ret);
    switch (errorStatus) {
    case SSL_ERROR_ZERO_RETURN: {
        cerr << "SSL_ERROR_ZERO_RETURN" << endl;
        break;
    }
    case SSL_ERROR_WANT_READ: {
        cerr << "SSL_ERROR_WANT_READ" << endl;
        break;
    }
    case SSL_ERROR_WANT_WRITE: {
        cerr << "SSL_ERROR_WANT_WRITE" << endl;
        break;
    }
    case SSL_ERROR_WANT_CONNECT: {
        cerr << "SSL_ERROR_WANT_CONNECT" << endl;
        break;
    }
    case SSL_ERROR_WANT_ACCEPT: {
        cerr << "SSL_ERROR_WANT_ACCEPT" << endl;
        break;
    }
    case SSL_ERROR_WANT_X509_LOOKUP: {
        cerr << "SSL_ERROR_WANT_X509_LOOKUP" << endl;
        break;
    }
    case SSL_ERROR_SYSCALL: {
        cerr << "SSL_ERROR_SYSCALL" << endl;
        break;
    }
    case SSL_ERROR_SSL: {
        cerr << "SSL_ERROR_SSL" << endl;
        break;
    }
    }
    ERR_clear_error();
    return true;
}