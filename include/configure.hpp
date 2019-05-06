//
// Created by a on 11/17/18.
//

#ifndef GENERALDEDUPSYSTEM_CONFIGURE_HPP
#define GENERALDEDUPSYSTEM_CONFIGURE_HPP

#include <vector>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define SIMPLE_CHUNKER 0
#define RABIN_CHUNKER 1
#define FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker

class Configure {
private:
    // following settings configure by macro set
    uint64_t _runningType;      // localDedup \ serverDedup

    // chunking settings
    uint64_t _chunkingType;     // varSize \ fixedSize \ simple
    uint64_t _maxChunkSize;
    uint64_t _minChunkSize;
    uint64_t _averageChunkSize;
    uint64_t _slidingWinSize;
    uint64_t _segmentSize;  // if exist segment function
    uint64_t _ReadSize;     //128M per time

    // message queue settings
    uint64_t _mqCnt;
    uint64_t *_messageQueueCnt;
    uint64_t *_messageQueueUnitSize;

    // key management settings
    uint64_t _keyServerNumber;
    std::vector<std::string> _keyServerIP;
    std::vector<int> _keyServerPort;
    uint64_t _keyBatchSizeMax;
    uint64_t _keyBatchSizeMin;
    uint64_t _keyCacheSize;

    //muti thread settings
    int _encodeThreadLimit;
    int _keyClientThreadLimit;
    int _keyServerThreadLimit;
    int _senderThreadLimit;
    int _receiverThreadLimit;
    int _decoderThreadLimit;
    int _dataSRThreadLimit;
    int _retriverThreadLimit;
    int _dedupCoreThreadLimit;
    int _storageCoreThreadLimit;

    //POW settings
    int _POWQuoteType; //0x00 linkable; 0x01 unlinkable
    int _POWIasVersion;
    std::string _POWServerIp;
    int _POWServerPort;
    std::string _POWEnclaveName;
    std::string _POWSPID;
    int _POWIasServerType;     //0 for develop; 1 for production

    //km enclave settings
    int _KMQuoteType; //0x00 linkable; 0x01 unlinkable
    int _KMIasVersion;
    std::string _KMServerIp;
    int _KMServerPort;
    std::string _KMEnclaveName;
    std::string _KMSPID;
    int _KMIasServerType;     //0 for develop; 1 for production

    // storage management settings
    uint64_t _storageServerNumber;
    std::vector<std::string> _storageServerIP;
    std::vector<int> _storageServerPort;
    uint64_t _maxContainerSize;

    //server setting
    std::string _fileRecipeRootPath;
    std::string _keyRecipeRootPath;
    std::string _containerRootPath;
    std::string _fp2ChunkDBName;
    std::string _fn2MetaDBame;

    //client settings
    int _clientID;
    int _sendChunkBatchSize;

	//timer settings
	double _timeOutScale;

    // any additional settings

public:


    //  Configure(std::ifstream& confFile); // according to setting json to init configure class
    Configure(std::string path);

    Configure();

    ~Configure();

    void readConf(std::string path);

    uint64_t getRunningType();

    // chunking settings
    uint64_t getChunkingType();

    uint64_t getMaxChunkSize();

    uint64_t getMinChunkSize();

    uint64_t getAverageChunkSize();

    uint64_t getSlidingWinSize();

    uint64_t getSegmentSize();

    uint64_t getReadSize();

    // message queue settions
    uint64_t getMessageQueueCnt(int index);

    uint64_t getMessageQueueUnitSize(int index);

    // key management settings
    uint64_t getKeyServerNumber();

    std::string getKeyServerIP(int index);
    //std::vector<std::string> getkeyServerIP();

    int getKeyServerPort(int index);
    //std::vector<int> getKeyServerPort();

    uint64_t getKeyBatchSizeMin();

    uint64_t getKeyBatchSizeMax();

    uint64_t getKeyCacheSize();

    //muti thread settings
    int getEncoderThreadLimit();
    int getKeyClientThreadLimit();
    int getKeyServerThreadLimit();

    int getSenderThreadLimit();
    int getReceiverThreadLimit();
    int getDecoderThreadLimit();
    int getDataSRThreadLimit();
    int getRetriverThreadLimit();
    int getDedupCoreThreadLimit();
    int getStorageCoreThreadLimit();


    //pow settings
    int getPOWQuoteType();
    int getPOWIASVersion();
    std::string getPOWServerIP();
    int getPOWServerPort();
    std::string getPOWEnclaveName();
    std::string getPOWSPID();
    int getPOWIASServerType();

    //km settings
    int getKMQuoteType();
    int getKMIASVersion();
    std::string getKMServerIP();
    int getKMServerPort();
    std::string getKMEnclaveName();
    std::string getKMSPID();
    int getKMIASServerType();

    // storage management settings
    uint64_t getStorageServerNumber();

    std::string getStorageServerIP(int index);
    //std::vector<std::string> getStorageServerIP();

    int getStorageServerPort(int index);
    //std::vector<int> getStorageServerPort();

    uint64_t getMaxContainerSize();

    //server settings
    std::string getFileRecipeRootPath();
    std::string getKeyRecipeRootPath();
    std::string getContainerRootPath();
    std::string getFp2ChunkDBName();
    std::string getFn2MetaDBame();

    //client settings
    int getClientID();
    int getSendChunkBatchSize();

    //timer settings
    double getTimeOutScale();

};



#endif //GENERALDEDUPSYSTEM_CONFIGURE_HPP
