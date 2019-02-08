//
// Created by a on 2/5/19.
//

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <iostream>

using namespace std;

class CryptoCrimitive{
private:
    RSA* r;
public:

};


bool generate_key()
{
    int             ret = 0;
    RSA             *r = NULL;
    BIGNUM          *bne = NULL;
    BIO             *bp_public = NULL, *bp_private = NULL;

    int             bits = 2048;
    unsigned long   e = RSA_F4;

    // 1. generate rsa key
    bne = BN_new();
    ret = BN_set_word(bne,e);
    if(ret != 1){
        goto free_all;
    }

    r = RSA_new();
    ret = RSA_generate_key_ex(r, bits, bne, NULL);
    if(ret != 1){
        goto free_all;
    }

    // 2. save public key
    bp_public = BIO_new_file("public.pem", "w+");
    ret = PEM_write_bio_RSAPublicKey(bp_public, r);
    if(ret != 1){
        goto free_all;
    }

    // 3. save private key
    bp_private = BIO_new_file("private.pem", "w+");
    ret = PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL);

    cout<<RSA_size(r);

    // 4. free
    free_all:

    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);

    return (ret == 1);
}

void fun1(char **argc) {
    OpenSSL_add_all_algorithms();
    EVP_PKEY *prikey = nullptr, *pubkey = nullptr;
    BIO *prifile = nullptr, *pubfile = nullptr;
    RSA *pubrsa = nullptr, *prirsa = nullptr, *newra = nullptr;

    prifile = BIO_new_file(argc[1], "r");

    char passwd[] = "1111";

    prikey = PEM_read_bio_PrivateKey(prifile, nullptr, 0, passwd);;

    prirsa = EVP_PKEY_get1_RSA(prikey);

    cout << EVP_PKEY_size(prikey) << endl;

    cout << RSA_size(prirsa) << endl;
}

void fun2(char **argc) {
    OpenSSL_add_all_algorithms();
    EVP_PKEY *prikey = nullptr, *pubkey = nullptr;
    BIO *prifile = nullptr, *pubfile = nullptr;
    RSA *pubrsa = nullptr, *prirsa = nullptr, *newra = nullptr;
    const BIGNUM *n = nullptr, *d = nullptr, *e = nullptr;

    prifile = BIO_new_file(argc[1], "r");
    pubfile = BIO_new_file(argc[2], "r");

    char passwd[] = "1111";
    prirsa = RSA_new();
    pubrsa = RSA_new();
    prirsa = PEM_read_bio_RSAPrivateKey(prifile, nullptr, 0, passwd);
    pubrsa = PEM_read_bio_RSAPublicKey(pubfile, nullptr, 0, NULL);

    cout<<RSA_size(prirsa)<<endl;
    unsigned char plaintext[]="123456",buffer[256],ciphertext[256];

    cout<<RSA_private_encrypt(6,plaintext,ciphertext,prirsa,RSA_PKCS1_PADDING)<<endl;
    cout<<ciphertext<<endl;
    cout<<RSA_public_decrypt(256,ciphertext,buffer,pubrsa,RSA_PKCS1_PADDING)<<endl;
    cout<<buffer<<endl;
}

int main(int argv,char **argc){

    fun1(argc);
    return 0;
}