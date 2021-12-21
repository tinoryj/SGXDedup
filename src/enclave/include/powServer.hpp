#ifndef SGXDEDUP_POWSERVER_HPP
#define SGXDEDUP_POWSERVER_HPP

#include "base64.h"
#include "byteorder.h"
#include "configure.hpp"
#include "crypto.h"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "enclaveSession.hpp"
#include "hexutil.h"
#include "iasrequest.h"
#include "json.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>
#include <openssl/err.h>

extern Configure config;

using namespace std;

class powServer {
private:
    CryptoPrimitive* cryptoObj_;

public:
    powServer();
    ~powServer();
    map<int, enclaveSession*> sessions;
    void closeSession(int fd);
    bool process_signedHash(enclaveSession* session, u_char* mac, u_char* hashList, int chunkNumber);
};

#endif //SGXDEDUP_POWSERVER_HPP
