#include "keyServer.hpp"

extern Configure config;

keyServer::keyServer()
{
    rsa_ = RSA_new();
    key_ = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(key_, &rsa_, NULL, passwd);
    RSA_get0_key(rsa_, &keyN_, nullptr, &keyD_);
    BIO_free_all(key_);
    RSA_free(rsa_);
}

keyServer::~keyServer() {}

void keyServer::run(Socket socket)
{
    string keyn, keyd;
    int lenKeyn, lenKeyd;
    keyn.resize(2048);
    keyd.resize(2048);
    lenKeyn = BN_bn2bin(keyN_, (unsigned char*)keyn.c_str());
    lenKeyd = BN_bn2bin(keyD_, (unsigned char*)keyd.c_str());
    keyn.resize(lenKeyn);
    keyd.resize(lenKeyd);
    kmClient* client = new kmClient(keyn, keyd);
    if (!client->init(socket)) {
        cerr << "keyServer: enclave not truster" << endl;
        pthread_exit(NULL);
    }
    while (true) {
        u_char hash[CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!socket.Recv(hash, recvSize)) {
            socket.finish();
            pthread_exit(NULL);
        }
        if (recvSize != CHUNK_HASH_SIZE) {
            cerr << "key manager recv chunk hash error : hash size wrong" << endl;
        } else {
            cerr << "KeyServer : correct recv chunk hash" << endl;
        }
        u_char key[CHUNK_ENCRYPT_KEY_SIZE];
        cerr << "KeyServer : start enclave request" << endl;
        client->request(hash, CHUNK_HASH_SIZE, key, CHUNK_ENCRYPT_KEY_SIZE);
        cerr << "KeyServer : enclave request done" << endl;
        if (!socket.Send(key, CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            socket.finish();
            pthread_exit(NULL);
        } else {
            cerr << "KeyServer : send back chunk key over" << endl;
        }
    }
}