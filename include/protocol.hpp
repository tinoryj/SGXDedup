//
// Created by a on 1/23/19.
//

#ifndef GENERALDEDUPSYSTEM_PROTOCOL_HPP
#define GENERALDEDUPSYSTEM_PROTOCOL_HPP

#include "seriazation.hpp"

//client-server network protocol
#define SGX_RA_MSG01                    0X00
#define SGX_RA_MSG2                     0X01
#define SGX_RA_MSG3                     0X02
#define SGX_RA_MSG4                     0X03
#define SGX_SIGNED_HASH                 0x04
#define POW_REQUEST                     0X05
#define CLIENT_UPLOAD_CHUNK             0X06
#define CLIENT_DOWNLOAD_CHUNK           0X07
#define SERVER_REQUIRED_CHUNK           0X08
#define CLIENT_UPLOAD_RECIPE            0X09
#define CLIENT_DOWNLOAD_RECIPE          0X0a
#define ERROR_RESEND                    0x0b
#define ERROR_CLOSE                     0x0c
#define SUCCESS                         0x0d

//dupCore-storageCore protocol
#define SAVE_CHUNK
#define SAVE_RECIPE

struct networkMsg_t {
    int _type;
    string _data;

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & _type;
        ar & _data;
    }
};



#endif //GENERALDEDUPSYSTEM_PROTOCOL_HPP
