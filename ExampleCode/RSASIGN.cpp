#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/sha.h"
#include "openssl/evp.h"
#include <string>
#include <iostream>
#include <algorithm>



using namespace std;


string sign(string d,RSA* rsa){
    string ans;
    unsigned int len=1024;
    ans.resize(len);
    int status=RSA_sign(NID_hmacWithSHA1,(unsigned char*)&d[0],
    d.length(),(unsigned char*)&ans[0],&len,rsa);
    ans.resize(len);
    if(status==1)cout<<"sign successed\n";
    else cout<<"sign failed\n";
    return ans;
}

void verify(string d,string h,RSA* rsa){
    int status=RSA_verify(NID_hmacWithSHA1,(unsigned char*)&d[0],
    d.length(),(unsigned char*)&h[0],h.length(),rsa);
    if(status)cout<<"verify successed\n";
    else cout<<"verify failed\n";
}

int main() {
    string data,Hash;
    EVP_MD_CTX *ctx=EVP_MD_CTX_create();
    EVP_MD_CTX_init(ctx);
    RSA *rsapub=RSA_new(),*rsapri=RSA_new();
    BIO *file=BIO_new_file("key/server.key","r");
    char passwd[5]="1111";passwd[4]='\0';
    PEM_read_bio_RSAPublicKey(file,&rsapub,NULL,(void *)passwd);
    PEM_read_bio_RSAPrivateKey(file,&rsapri,NULL,(void*)passwd);

    EVP_DigestInit(ctx,EVP_sha1());
    for(int i=0;i<1;i++){
        cin>>data;
        EVP_DigestUpdate(ctx,(void*)&data[0],data.length());
    }
    Hash.resize(1024);
    int len;
    EVP_DigestFinal(ctx,(unsigned char*)&Hash[0],(unsigned int*)&len);
    cout<<sizeof(RSA);
    //verify(Hash,sign(Hash,rsapri),rsapub);
    return 0;
}