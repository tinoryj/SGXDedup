//
// Created by a on 11/17/18.
//

#include "CryptoPrimitive.hpp"


CryptoPrimitive::CryptoPrimitive(int cryptoType){
    _cryptoType = cryptoType;

    if (_cryptoType == HIGH_SEC_PAIR_TYPE) {
        EVP_MD_CTX_init(&_mdCTX);
        _md = EVP_sha256();
        _hashSize = 32;
        EVP_CIPHER_CTX_init(&_cipherctx);

        _cipher = EVP_aes_256_cbc();
        _keySize = 32;
        _blockSize = 16;

    }

    if (_cryptoType == LOW_SEC_PAIR_TYPE) {
        EVP_MD_CTX_init(&_mdCTX);

        _md = EVP_md5();
        _hashSize = 16;

        EVP_CIPHER_CTX_init(&_cipherctx);

        _cipher = EVP_aes_128_cbc();
        _keySize = 16;
        _blockSize = 16;

        _iv = (unsigned char *) malloc(sizeof(unsigned char) * _blockSize);
        memset(_iv, 0, _blockSize);
    }

    if (_cryptoType == SHA256_TYPE) {
        EVP_MD_CTX_init(&_mdCTX);

        _md = EVP_sha256();
        _hashSize = 32;

        _keySize = -1;
        _blockSize = -1;
    }

    if (_cryptoType == SHA1_TYPE) {
        EVP_MD_CTX_init(&_mdCTX);
        _md = EVP_sha1();
        _hashSize = 20;

        _keySize = -1;
        _blockSize = -1;

    }
}
CryptoPrimitive::~CryptoPrimitive(){
    if ((_cryptoType == HIGH_SEC_PAIR_TYPE) || (_cryptoType == LOW_SEC_PAIR_TYPE)) {
        EVP_MD_CTX_cleanup(&_mdCTX);

        EVP_CIPHER_CTX_cleanup(&_cipherctx);
        free(_iv);
    }

    if ((_cryptoType == SHA256_TYPE) || (_cryptoType == SHA1_TYPE)) {
        EVP_MD_CTX_cleanup(&_mdCTX);
    }
}



bool CryptoPrimitive::generaHash(vector<string>data,string &hash){
    int hashSize;

    EVP_DigestInit_ex(&_mdCTX, _md, NULL);

    for(auto it:data) {
        EVP_DigestUpdate(&_mdCTX, it.c_str(), it.length());
    }

    hash.resize(_hashSize);
    EVP_DigestFinal_ex(&_mdCTX,(unsigned char*)&hash[0], (unsigned int*) &hashSize);

    if (hashSize != _hashSize) {
        /*fprintf(stderr, "Error: the size of the generated hash (%d bytes) does not match with the expected one (%d bytes)!\n",
                hashSize, _hashSize);*/

        return 0;
    }
    return 1;
}

bool CryptoPrimitive::generaHash(string data,string &hash){
    int hashSize;

    EVP_DigestInit_ex(&_mdCTX, _md, NULL);

    EVP_DigestUpdate(&_mdCTX, data.c_str(), data.length());

    hash.resize(_hashSize);
    EVP_DigestFinal_ex(&_mdCTX,(unsigned char*)&hash[0], (unsigned int*) &hashSize);

    if (hashSize != _hashSize) {
        /*fprintf(stderr, "Error: the size of the generated hash (%d bytes) does not match with the expected one (%d bytes)!\n",
                hashSize, _hashSize);*/

        return 0;
    }
    return 1;
}


bool CryptoPrimitive::encryptWithKey(string data,string key,string& cipherText) {
    int cipherTextSize, cipherTextTailSize;

    /**********************************/
    char* buffer=new char[4096];
    /**********************************/

    if (data.length() % _blockSize != 0) {
        std::cerr<< "Error: the size of the input data ("<<data.length()<<" bytes) is not a multiple of that of encryption block unit ( "<<_blockSize<<" bytes)!\n";
        return 0;
    }

    EVP_EncryptInit_ex(&_cipherctx, _cipher, NULL, (unsigned char*)key.c_str(), _iv);
    EVP_CIPHER_CTX_set_padding(&_cipherctx, 0);
    EVP_EncryptUpdate(&_cipherctx, (unsigned char*)buffer, &cipherTextSize, (unsigned char*)data.c_str(), data.length());
    EVP_EncryptFinal_ex(&_cipherctx, (unsigned char*)buffer + cipherTextSize, &cipherTextTailSize);
    cipherTextSize += cipherTextTailSize;



    if (cipherTextSize != data.length()) {
        /*fprintf(stderr, "Error: the size of the cipher output (%d bytes) does not match with that of the input (%d bytes)!\n",
                cipherTextSize, data.length());*/

        return 0;
    }

    cipherText=buffer;

    return 1;
}

bool CryptoPrimitive::decryptWithKey(string cipherText,string key,string &plaintText){
    int plaintTextSize, plaintTextTailSize;

    /******************************/
    char *buffer=new char[4096];
    /******************************/

    if (cipherText.length() % _blockSize != 0) {
        /*fprintf(stderr, "Error: the size of the input data (%d bytes) is not a multiple of that of encryption block unit (%d bytes)!\n",
                cipherText.length(), _blockSize);
                */

        return 0;
    }

    EVP_DecryptInit_ex(&_cipherctx, _cipher, NULL, (unsigned char*)key.c_str(), _iv);
    EVP_CIPHER_CTX_set_padding(&_cipherctx, 0);
    EVP_DecryptUpdate(&_cipherctx, (unsigned char*)buffer, &plaintTextSize, (unsigned char*)cipherText.c_str(), cipherText.length());
    EVP_DecryptFinal_ex(&_cipherctx,(unsigned char*) buffer + plaintTextSize, &plaintTextTailSize);
    plaintTextSize += plaintTextTailSize;

    if (plaintTextSize != cipherText.length()) {
        /*
        fprintf(stderr, "Error: the size of the plaintText output (%d bytes) does not match with that of the original data (%d bytes)!\n",
                plaintTextSize, cipherText.length());
        */
        return 0;
    }

    plaintText=buffer;
    return 1;
}


int CryptoPrimitive::getHashSize() {
    return _hashSize;
}

int CryptoPrimitive::getKeySize() {
    return _keySize;
}

int CryptoPrimitive::getBlockSize() {
    return _blockSize;
}
