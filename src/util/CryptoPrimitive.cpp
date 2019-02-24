#include "CryptoPrimitive.hpp"

bool CryptoPrimitive::readKeyFromPEM(string pubfile,string prifile, string passwd) {
    BIO *f = nullptr;

    _pubKeySet = this->setPubKey(pubfile);
    _priKeySet = this->setPriKey(prifile, passwd);

    return true;
}

CryptoPrimitive::CryptoPrimitive() {
    _pubKey = nullptr;
    _priKey = nullptr;
    _symKey = nullptr;
    _iv = nullptr;
    _pubKeySet = _priKeySet = _symKeySet = _ivSet = false;

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
}

CryptoPrimitive::CryptoPrimitive(string pubFile, string priFile, string passwd) {
    _pubKey = nullptr;
    _priKey = nullptr;
    _symKey = nullptr;
    _iv = nullptr;
    _pubKeySet = _priKeySet = _symKeySet = _ivSet = false;
    if (!this->readKeyFromPEM(pubFile, priFile, passwd)) {
        cerr << "CryptoPrimitive initial failed\n";
    }
}

bool CryptoPrimitive::setSymKey(const char *key, unsigned int len,const char *iv, int ivLen) {
    if (_symKey != nullptr) {
        delete _symKey;
    }

    if (_iv != nullptr) {
        delete _iv;
    }

    _symKeyLen = len;
    _symKey = new unsigned char[len];

    _ivLen = ivLen;
    _iv = new unsigned char[ivLen];

    if (_symKey == nullptr) {
        cerr << "Mem out\n";
        return false;
    }

    if (_iv == nullptr) {
        cerr << "Mem out\n";
        return false;
    }

    memcpy(_symKey, key, _symKeyLen);
    memcpy(_iv, iv, _ivLen);

    _symKeySet = _ivSet = true;

    return true;
}

bool CryptoPrimitive::setPubKey(string pubFile) {
    BIO *f;
    EVP_PKEY *tmpkey;

    if (pubFile.length() != 0) {
        f = BIO_new_file(pubFile.c_str(), "r");

        if (f == nullptr) {
            cerr << "Can not open " << pubFile << " for CryptoPrimitive to read key\n";
            return _pubKeySet;
        }

        PEM_read_bio_PUBKEY(f, &tmpkey, nullptr, nullptr);

        if (tmpkey == nullptr) {
            cerr << "Can not read public key form " << pubFile << endl;
            return _pubKeySet;
        }

        _pubKey = tmpkey;

        BIO_free(f);
    }

    return true;
}

bool CryptoPrimitive::setPriKey(string priFile, string passwd) {
    BIO *f;
    EVP_PKEY *tmpkey;

    if (priFile.length() != 0) {
        f = BIO_new_file(priFile.c_str(), "r");

        if (f == nullptr) {
            cerr << "Can not open " << priFile << " for CryptoPrimitive to read key\n";
            return _priKeySet;
        }

        PEM_read_bio_PUBKEY(f, &tmpkey, nullptr, nullptr);

        if (tmpkey == nullptr) {
            cerr << "Can not read public key form " << priFile << endl;
            return _priKeySet;
        }

        _priKey = tmpkey;

        BIO_free(f);

    }

    return true;
}

bool CryptoPrimitive::encrypt(string &plaintext, string &ciphertext,const EVP_CIPHER *type) {

    if (!_symKeySet || !_ivSet)return false;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    ciphertext.clear();
    ciphertext.resize(plaintext.length() + 1024);
    int cipherlen, len;

    if (ctx == nullptr) {
        cerr << "can not initial cipher ctx\n";
        return false;
    }

    if (EVP_EncryptInit_ex(ctx, type, nullptr, this->_symKey, this->_iv) != 1) {
        cerr << "encrypt error\n";
        EVP_CIPHER_CTX_cleanup(ctx);
        return false;
    }

    if (EVP_EncryptUpdate(ctx, (unsigned char *) &ciphertext[0], &len, (unsigned char *) &plaintext[0],
                          plaintext.length()) != 1) {
        cerr << "encrypt error\n";
        EVP_CIPHER_CTX_cleanup(ctx);
        return false;
    }

    cipherlen = len;

    if (EVP_EncryptFinal_ex(ctx, (unsigned char *) &ciphertext[cipherlen], &len) != 1) {
        cerr << "encrypt error\n";
        EVP_CIPHER_CTX_cleanup(ctx);
        return false;
    }

    ciphertext.resize(cipherlen + len);

    EVP_CIPHER_CTX_cleanup(ctx);
    return true;
}

bool CryptoPrimitive::decrypt(string &ciphertext, string &plaintext,const EVP_CIPHER *type) {

    if (!_symKeySet || !_ivSet)return false;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    plaintext.clear();
    plaintext.resize(ciphertext.length() + 1024);
    int plaintlen, len;

    if (ctx == nullptr) {
        cerr << "can not initial cipher ctx\n";
        return false;
    }

    if (EVP_DecryptInit_ex(ctx, type, nullptr, this->_symKey, this->_iv) != 1) {
        cerr << "decrypt error\n";
        EVP_CIPHER_CTX_cleanup(ctx);
        return false;
    }

    if (EVP_DecryptUpdate(ctx, (unsigned char *) &plaintext[0], &len, (unsigned char *) &ciphertext[0],
                          ciphertext.length()) != 1) {
        cerr << "decrypt error\n";
        EVP_CIPHER_CTX_cleanup(ctx);
        return false;
    }

    plaintlen = len;

    if (EVP_DecryptFinal_ex(ctx, (unsigned char *) &ciphertext[plaintlen], &len) != 1) {
        cerr << "decrypt error\n";
        EVP_CIPHER_CTX_cleanup(ctx);
        return false;
    }

    plaintext.resize(plaintlen + len);

    EVP_CIPHER_CTX_cleanup(ctx);
    return true;
}

bool CryptoPrimitive::cmac(vector<string> &messge, string &mac, const EVP_CIPHER *type) {
    CMAC_CTX *ctx = CMAC_CTX_new();

    int keyLen;
    if(type==EVP_aes_128_cbc()){
        keyLen=16;
    }
    else keyLen=32;

    if (ctx == nullptr) {
        cerr << "error initial cmac ctx\n";
        return false;
    }


    if (CMAC_Init(ctx, _symKey, keyLen, type, nullptr) != 1) {
        cerr << "cmac error\n";
        CMAC_CTX_cleanup(ctx);
        return false;
    }

    for(auto it:messge){
        CMAC_Update(ctx,(void*)&it[0],it.length());
    }

    mac.clear();
    mac.resize(16);
    size_t maclen;


    if (CMAC_Final(ctx, (unsigned char *) &mac[0], &maclen) != 1) {
        cerr << "cmac error\n";
        CMAC_CTX_cleanup(ctx);
        return false;
    }

    mac.resize(maclen);

    CMAC_CTX_cleanup(ctx);
    return true;
}

bool CryptoPrimitive::message_digest(string &message, string &hash, const EVP_MD *type) {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

    if (ctx == nullptr) {
        cerr << "error initial MD ctx\n";
        return false;
    }

    if (EVP_DigestInit_ex(ctx, type, nullptr) != 1) {
        cerr << "hash error\n";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    if (EVP_DigestUpdate(ctx, (void *) &message[0], message.length()) != 1) {
        cerr << "hash error\n";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    hash.clear();
    unsigned int mdlen;
    hash.resize(64);

    if (EVP_DigestFinal_ex(ctx, (unsigned char *) &hash[0], &mdlen) != 1) {
        cerr << "hash error\n";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    hash.resize(mdlen);

    EVP_MD_CTX_free(ctx);
    return true;
}

/* aes cbc method */
bool CryptoPrimitive::cbc128_encrypt(string &plaintext, string &ciphertext) {
    return encrypt(plaintext, ciphertext, EVP_aes_128_cbc());
}
bool CryptoPrimitive::cbc128_decrypt(string &ciphertext, string &plaintext) {
    return decrypt(ciphertext, plaintext, EVP_aes_128_cbc());
}
bool CryptoPrimitive::cbc256_encrypt(string &plaintext, string &ciphertext) {
    return encrypt(plaintext, ciphertext, EVP_aes_256_cbc());
}
bool CryptoPrimitive::cbc256_decrypt(string &ciphertext, string &plaintext) {
    return decrypt(ciphertext, plaintext, EVP_aes_256_cbc());
}

/* aes cfb method */
bool CryptoPrimitive::cfb128_encrypt(string &plaintext, string &ciphertext) {
    return encrypt(plaintext, ciphertext, EVP_aes_128_cfb());
}
bool CryptoPrimitive::cfb128_decrypt(string &ciphertext, string &plaintext) {
    return decrypt(ciphertext, plaintext, EVP_aes_128_cfb());
}
bool CryptoPrimitive::cfb256_encrypt(string &plaintext, string &ciphertext) {
    return encrypt(plaintext, ciphertext, EVP_aes_256_cfb());
}
bool CryptoPrimitive::cfb256_decrypt(string &ciphertext, string &plaintext) {
    return decrypt(ciphertext, plaintext, EVP_aes_256_cfb());
}


/* aes ofb method */
bool CryptoPrimitive::ofb128_encrypt(string &plaintext, string &ciphertext) {
    return encrypt(plaintext, ciphertext, EVP_aes_128_ofb());
}
bool CryptoPrimitive::ofb128_decrypt(string &ciphertext, string &plaintext) {
    return decrypt(ciphertext, plaintext, EVP_aes_128_ofb());
}
bool CryptoPrimitive::ofb256_encrypt(string &plaintext, string &ciphertext) {
    return encrypt(plaintext, ciphertext, EVP_aes_256_ofb());
}
bool CryptoPrimitive::ofb256_decrypt(string &ciphertext, string &plaintext) {
    return decrypt(ciphertext, plaintext, EVP_aes_256_ofb());
}

bool CryptoPrimitive::cmac128(vector<string> &message, string &mac) {
    return cmac(message, mac, EVP_aes_128_cbc());
}

bool CryptoPrimitive::cmac256(vector<string> &message, string &mac) {
    return cmac(message, mac, EVP_aes_256_cbc());
}

bool CryptoPrimitive::sha1_digest(string &message, string &digest) {
    return message_digest(message, digest, EVP_sha1());
}

bool CryptoPrimitive::sha256_digest(string &message, string &digest) {
    return message_digest(message, digest, EVP_sha256());
}

bool CryptoPrimitive::sha512_digest(string &message, string &digest) {
    return message_digest(message, digest, EVP_sha512());
}

bool CryptoPrimitive::recipe_encrypt(keyRecipe_t &recipe, string &ans) {
    string recipeBuffer;
    serialize(recipe, recipeBuffer);
    if(!this->cbc256_encrypt(recipeBuffer, ans)){
        cerr<<"recipe encrypt error\n";
        return false;
    }
    return true;
}

bool CryptoPrimitive::recipe_decrypt(string buffer, keyRecipe_t &recipe) {
    string recipeBuffer;
    if (!this->cbc256_decrypt(buffer, recipeBuffer)) {
        cerr << "recipe decrypt error\n";
        return false;
    }
    deserialize(recipeBuffer, recipe);
    return true;
}

bool CryptoPrimitive::chunk_encrypt(Chunk &chunk) {
    string cipherChunk, plaintChunk;
    plaintChunk = chunk.getLogicData();
    string key;
    key=chunk.getEncryptKey();
    this->setSymKey(key.c_str(),key.length(),key.c_str(),key.length());
    if (!this->cbc256_encrypt(plaintChunk, cipherChunk)) {
        cerr << "chunk encrypt error\n";
        return false;
    }
    chunk.editLogicData(cipherChunk, cipherChunk.length());
    return true;
}

bool CryptoPrimitive::chunk_decrypt(Chunk &chunk) {
    string cipherChunk, plaintChunk;
    cipherChunk = chunk.getLogicData();
    string key;
    key=chunk.getEncryptKey();
    this->setSymKey(key.c_str(),key.length(),key.c_str(),key.length());
    if (!this->cbc256_decrypt(cipherChunk, plaintChunk)) {
        cerr << "chunk decrypt error\n";
        return false;
    }
    chunk.editLogicData(plaintChunk, plaintChunk.length());
    return true;
}
