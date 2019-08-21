#include "keyServer.hpp"
#include <sys/time.h>
extern Configure config;

// struct timeval timestart;
// struct timeval timeend;

void PRINT_BYTE_ARRAY_KEY_SERVER(
    FILE* file, void* mem, uint32_t len)
{
    if (!mem || !len) {
        fprintf(file, "\n( null )\n");
        return;
    }
    uint8_t* array = (uint8_t*)mem;
    fprintf(file, "%u bytes:\n{\n", len);
    uint32_t i = 0;
    for (i = 0; i < len - 1; i++) {
        fprintf(file, "0x%x, ", array[i]);
        if (i % 8 == 7)
            fprintf(file, "\n");
    }
    fprintf(file, "0x%x ", array[i]);
    fprintf(file, "\n}\n");
}

keyServer::keyServer(Socket socket)
{
    rsa_ = RSA_new();
    key_ = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(key_, &rsa_, NULL, passwd);
    RSA_get0_key(rsa_, &keyN_, nullptr, &keyD_);

    u_char keynBuffer[4096];
    u_char keydBuffer[4096];
    int lenKeyn, lenKeyd;
    BN_print_fp(stderr, keyD_);
    cerr << endl;
    BN_print_fp(stderr, keyN_);
    cerr << endl;
    lenKeyn = BN_bn2bin(keyN_, keynBuffer);
    lenKeyd = BN_bn2bin(keyD_, keydBuffer);
    string keyn((char*)keynBuffer, lenKeyn);
    string keyd((char*)keydBuffer, lenKeyd);
    client = new kmClient(keyn, keyd);
    if (!client->init(socket)) {
        cerr << "keyServer : enclave not truster" << endl;
        return;
    } else {
        cerr << "KeyServer : enclave trusted" << endl;
    }
    socket.finish();
}

keyServer::~keyServer()
{
    BIO_free_all(key_);
    RSA_free(rsa_);
    delete client;
}

void keyServer::run(Socket socket)
{

    while (true) {
        u_char hash[config.getKeyBatchSize() * CHUNK_HASH_SIZE];
        int recvSize = 0;
        if (!socket.Recv(hash, recvSize)) {
            socket.finish();
            return;
        }

        if ((recvSize % CHUNK_HASH_SIZE) != 0) {
            cerr << "key manager recv chunk hash error : hash size wrong" << endl;
            socket.finish();
            return;
        }
        int recvNumber = recvSize / CHUNK_HASH_SIZE;
        cerr << "KeyServer : recv hash number = " << recvNumber << endl;

        u_char key[recvNumber * CHUNK_ENCRYPT_KEY_SIZE];

        // gettimeofday(&timestart, 0);

        multiThreadMutex_.lock();
        for (int i = 0; i < recvNumber; i++) {
            client->request(hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE, key + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
            // cerr << "chunk " << i << " :" << endl;
            // PRINT_BYTE_ARRAY_KEY_SERVER(stderr, hash + i * CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
            // PRINT_BYTE_ARRAY_KEY_SERVER(stderr, key + i * CHUNK_ENCRYPT_KEY_SIZE, CHUNK_ENCRYPT_KEY_SIZE);
        }
        multiThreadMutex_.unlock();

        // gettimeofday(&timeend, 0);
        // long diff = 1000000 * (timeend.tv_sec - timestart.tv_sec) + timeend.tv_usec - timestart.tv_usec;
        // printf("KeyServer : Compute time is %ld us\n", diff);

        if (!socket.Send(key, recvNumber * CHUNK_ENCRYPT_KEY_SIZE)) {
            cerr << "KeyServer : error send back chunk key to client" << endl;
            socket.finish();
            return;
        }
    }
}