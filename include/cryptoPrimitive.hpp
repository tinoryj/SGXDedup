#ifndef GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
#define GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
#include "configure.hpp"
#include "dataStructure.hpp"
#include "seriazation.hpp"
#include <bits/stdc++.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

using namespace std;

class CryptoPrimitive {
private:
    EVP_PKEY *pubKey_, *priKey_;

    unsigned char* symKey_;
    unsigned char* iv_;
    unsigned int symKeyLen_;
    unsigned int ivLen_;

    bool pubKeySet_;
    bool priKeySet_;
    bool symKeySet_;
    bool ivSet_;

    bool encrypt(string& plaintext, string& ciphertext, const EVP_CIPHER* type);
    bool decrypt(string& ciphertext, string& plaintext, const EVP_CIPHER* type);

    bool cmac(vector<string>& messge, string& mac, const EVP_CIPHER* type);
    bool message_digest(string& message, string& hash, const EVP_MD* type);
    /*
    bool pub_encrypt();
    bool pub_decrypt();
    bool pri_encrypt();
    bool pri_decrypt();
*/
    bool readKeyFromPEM(string pubfile, string prifile, string passwd);

public:
    CryptoPrimitive();

    CryptoPrimitive(string pubFile, string priFile, string passwd);

    bool setSymKey(const char* key, unsigned int len, const char* iv, int ivLen);
    bool setPriKey(string priFile, string passwd);
    bool setPubKey(string pubFile);

    bool cbc128_encrypt(string& plaintext, string& ciphertext);
    bool cbc128_decrypt(string& ciphertext, string& plaintext);
    bool cbc256_encrypt(string& plaintext, string& ciphertext);
    bool cbc256_decrypt(string& ciphertext, string& plaintext);

    bool cfb128_encrypt(string& plaintext, string& ciphertext);
    bool cfb128_decrypt(string& ciphertext, string& plaintext);
    bool cfb256_encrypt(string& plaintext, string& ciphertext);
    bool cfb256_decrypt(string& ciphertext, string& plaintext);

    bool ofb128_encrypt(string& plaintext, string& ciphertext);
    bool ofb128_decrypt(string& ciphertext, string& plaintext);
    bool ofb256_encrypt(string& plaintext, string& ciphertext);
    bool ofb256_decrypt(string& ciphertext, string& plaintext);

    bool cmac128(vector<string>& message, string& mac);
    bool cmac256(vector<string>& message, string& mac);

    bool sha1_digest(string& message, string& digest);
    bool sha256_digest(string& message, string& digest);
    bool sha512_digest(string& message, string& digest);

    /*
    bool base64_encode(string message, string code);
    bool base64_decode(string code, string message);
*/

    bool recipe_encrypt(KeyRecipeList_t& recipeList, string& encryptedRecipe);
    bool recipe_decrypt(string& encryptedRecipe, KeyRecipeList_t& recipeList);

    bool chunk_encrypt(Chunk_t& chunk);
    bool chunk_decrypt(Chunk_t& chunk);
};

#endif //GENERALDEDUPSYSTEM_CRYPTOPRIMITIVE_HPP
