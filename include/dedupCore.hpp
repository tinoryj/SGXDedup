#ifndef GENERALDEDUPSYSTEM_DEDUPCORE_HPP
#define GENERALDEDUPSYSTEM_DEDUPCORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataSR.hpp"
#include "dataStructure.hpp"
#include "database.hpp"
#include "messageQueue.hpp"
#include "protocol.hpp"
#include <bits/stdc++.h>
#include <boost/thread.hpp>

using namespace std;

class DedupCore {
private:
    CryptoPrimitive* cryptoObj_;
    bool dedupByHash(powSignedHash_t in, RequiredChunk_t& out);
    DataSR* dataSRObj_;

public:
    void run();
    DedupCore(DataSR* dataSRTemp);
    ~DedupCore();
    bool extractMQFromDataSR(EpollMessage_t& newMessage);
    bool insertMQToDataSR_CallBack(EpollMessage_t& newMessage);
};

#endif //GENERALDEDUPSYSTEM_DEDUPCORE_HPP
