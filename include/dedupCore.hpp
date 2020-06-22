#ifndef SGXDEDUP_DEDUPCORE_HPP
#define SGXDEDUP_DEDUPCORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "database.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>

using namespace std;

class DedupCore {
private:
    CryptoPrimitive* cryptoObj_;

public:
    DedupCore();
    ~DedupCore();
    bool dedupByHash(powSignedHash_t in, RequiredChunk_t& out);
};

#endif //SGXDEDUP_DEDUPCORE_HPP
