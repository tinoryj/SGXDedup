//
// Created by a on 11/17/18.
//

#include "keyServer.hpp"

keyServer::keyServer()
{
    _rsa = RSA_new();
    _key = NULL;
    _key = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(_key, &_rsa, NULL, passwd);
    RSA_get0_key(_rsa, &_keyN, nullptr, &_keyD);
    BIO_free_all(_key);
    RSA_free(_rsa);
}

keyServer::~keyServer() {}

void keyServer::run(Socket socket)
{
    string keyn, keyd;
    int lenKeyn, lenKeyd;
    keyn.resize(2048);
    keyd.resize(2048);
    lenKeyn = BN_bn2bin(_keyN, (unsigned char*)keyn.c_str());
    lenKeyd = BN_bn2bin(_keyD, (unsigned char*)keyd.c_str());
    keyn.resize(lenKeyn);
    keyd.resize(lenKeyd);
    kmClient* client = new kmClient(keyn, keyd);
    if (!client->init(socket)) {
        cerr << "keyServer: enclave not truster" << endl;
        return;
    }
    string hash, key;
    u_char hash[CHUNK_HASH_SIZE];
    while (1) {
        if (socket.Recv(hash)) {
            socket.finish();
            return;
        }
        client->request(hash, key);
        if (!socket.Send(key)) {
            socket.finish();
            return;
        }
    }
}