#ifndef SGXDEDUP_PROTOCOL_HPP
#define SGXDEDUP_PROTOCOL_HPP

#include <bits/stdc++.h>

//client-server network protocol
#define SGX_RA_MSG01 0
#define SGX_RA_MSG2 1
#define SGX_RA_MSG3 2
#define SGX_RA_MSG4 3
#define POW_CLOSE_SESSION 4
#define SGX_SIGNED_HASH 5
#define POW_REQUEST 6
#define CLIENT_UPLOAD_CHUNK 7
#define CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE 8
#define SERVER_REQUIRED_CHUNK 9
#define CLIENT_UPLOAD_RECIPE 10
#define CLIENT_DOWNLOAD_FILEHEAD 11
#define ERROR_RESEND 12
#define ERROR_CLOSE 13
#define SUCCESS 14
#define SGX_SIGNED_HASH_TO_DEDUPCORE 15
#define ERROR_FILE_NOT_EXIST 16
#define ERROR_CHUNK_NOT_EXIST 17
#define ERROR_CLIENT_CLOSE_CONNECT 18
#define CLIENT_EXIT 19
#define CLIENT_GET_KEY_SERVER_SK 20
#define RA_REQUEST 21
#define CLIENT_SET_LOGIN 22
#define CLIENT_SET_LOGOUT 23
#define KEY_SERVER_SESSION_KEY_UPDATE 24
#define KEY_SERVER_RA_REQUES 25
#define KEY_GEN_UPLOAD_CLIENT_INFO 26
#define KEY_GEN_UPLOAD_CHUNK_HASH 27
#define SERVER_JOB_DONE_EXIT_PERMIT 28
#define CLIENT_UPLOAD_DECRYPTED_RECIPE 29
#define CLIENT_DOWNLOAD_RECIPE_SIZE 30
#define CLIENT_UPLOAD_ENCRYPTED_RECIPE 31
#define CLIENT_DOWNLOAD_ENCRYPTED_RECIPE 32

using namespace std;

typedef struct _ra_msg4_struct {
    bool status;
    //sgx_platform_info_t platformInfoBlob;
} ra_msg4_t;

typedef struct {
    uint8_t signature_[16];
    vector<string> hash_;
} powSignedHash_t;

#endif //SGXDEDUP_PROTOCOL_HPP
