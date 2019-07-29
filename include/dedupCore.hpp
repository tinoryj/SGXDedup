#ifndef GENERALDEDUPSYSTEM_DEDUPCORE_HPP
#define GENERALDEDUPSYSTEM_DEDUPCORE_HPP

#include "cache.hpp"
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
    bool dedupStage1(powSignedHash_t in, RequiredChunk_t& out);
    bool dedupStage2(StorageChunkList_t& in);

public:
    DataSR* dataSRObj_;
    void run();
    DedupCore(DataSR* dataSRTemp);
    ~DedupCore();
    bool extractMQFromDataSR(EpollMessage_t& newMessage);
    bool insertMQToDataSR_CallBack(EpollMessage_t& newMessage);
};

#endif //GENERALDEDUPSYSTEM_DEDUPCORE_HPP
