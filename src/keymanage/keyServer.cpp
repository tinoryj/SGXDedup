#include "keyServer.hpp"
#include <sys/time.h>
extern Configure config;

struct timeval timestart;
struct timeval timeend;

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
        pthread_exit(0);
    }
    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!socket.Recv(hash, recvSize)) {
            socket.finish();
            pthread_exit(0);
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "key manager recv chunk hash error : hash size wrong" << endl;
            socket.finish();
            pthread_exit(0);
        }
        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        cerr << "KeyServer : recv hash number = " << recvNumber << endl;

        u_char key[recvNumber * CHUNK_ENCRYPT_KEY_SIZE];

        gettimeofday(&timestart, 0);
        for (int i = 0; i < recvNumber; i++) {
            client->request(hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE, key + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
        }

        gettimeofday(&timeend, 0);
        long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        double second = diff / 1000000.0;
        printf("Compute time is %ld us = %lf s\n", diff, second);

        if (!socket.Send(key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            socket.finish();
            pthread_exit(0);
        }
    }
}