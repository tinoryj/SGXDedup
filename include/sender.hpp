#ifndef SGXDEDUP_SENDER_HPP
#define SGXDEDUP_SENDER_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include "ssl.hpp"
#include <sgx_uae_service.h>
#include <sgx_ukey_exchange.h>

class Sender {
private:
    std::mutex mutexSocket_;
    ssl* powSecurityChannel_;
    ssl* dataSecurityChannel_;
    SSL* sslConnectionPow_;
    SSL* sslConnectionData_;
    int clientID_;
    messageQueue<Data_t>* inputMQ_;
    CryptoPrimitive* cryptoObj_;

public:
    Sender();

    ~Sender();

    //status define in protocol.hpp
    bool sendRecipe(Recipe_t request, RecipeList_t requestList, int& status);
    bool sendChunkList(ChunkList_t request, int& status);
    bool sendChunkList(char* requestBufferIn, int sendBufferSize, int sendChunkNumber, int& status);
    bool getKeyServerSK(u_char* SK);
    //for pow
    bool sendSGXmsg01(uint32_t& msg0, sgx_ra_msg1_t& msg1, sgx_ra_msg2_t*& msg2, int& status);
    bool sendSGXmsg3(sgx_ra_msg3_t* msg3, uint32_t sz, ra_msg4_t*& msg4, int& status);
    bool sendEnclaveSignedHash(powSignedHash_t& request, RequiredChunk_t& respond, int& status);
    bool sendLogOutMessage();
    bool sendLogInMessage();
    //send chunk when socket free
    void run();

    //general send data
    bool sendDataPow(char* request, int requestSize, char* respond, int& respondSize);
    bool sendEndFlag();
    bool insertMQFromPow(Data_t& newChunk);
    bool extractMQFromPow(Data_t& newChunk);
    bool editJobDoneFlag();
};

#endif //SGXDEDUP_SENDER_HPP
