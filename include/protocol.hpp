#ifndef GENERALDEDUPSYSTEM_PROTOCOL_HPP
#define GENERALDEDUPSYSTEM_PROTOCOL_HPP

#include <bits/stdc++.h>
//client-server network protocol
#define SGX_RA_MSG01 0X00
#define SGX_RA_MSG2 0X01
#define SGX_RA_MSG3 0X02
#define SGX_RA_MSG4 0X03
#define POW_CLOSE_SESSION 0x04
#define SGX_SIGNED_HASH 0x05
#define POW_REQUEST 0X06
#define CLIENT_UPLOAD_CHUNK 0X07
#define CLIENT_DOWNLOAD_CHUNK_WITH_RECIPE 0X08
#define SERVER_REQUIRED_CHUNK 0X09
#define CLIENT_UPLOAD_RECIPE 0X0a
#define CLIENT_DOWNLOAD_FILEHEAD 0X0b
#define ERROR_RESEND 0x0c
#define ERROR_CLOSE 0x0d
#define SUCCESS 0x0e
#define SGX_SIGNED_HASH_TO_DEDUPCORE 0x0f
#define ERROR_FILE_NOT_EXIST 0x10
#define ERROR_CHUNK_NOT_EXIST 0x11
#define ERROR_CLIENT_CLOSE_CONNECT 0x12

//dupCore-storageCore protocol
#define SAVE_CHUNK
#define SAVE_RECIPE
using namespace std;

typedef struct _ra_msg4_struct {
    bool status;
    //sgx_platform_info_t platformInfoBlob;
} ra_msg4_t;

typedef struct {
    uint8_t signature_[16];
    vector<string> hash_;
} powSignedHash_t;

#endif //GENERALDEDUPSYSTEM_PROTOCOL_HPP
