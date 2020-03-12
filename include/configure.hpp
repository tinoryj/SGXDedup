#ifndef GENERALDEDUPSYSTEM_CONFIGURE_HPP
#define GENERALDEDUPSYSTEM_CONFIGURE_HPP

#include <bits/stdc++.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;

#define BREAK_DOWN
#define HIGH_SECURITY
// #define STORAGE_SERVER_VERIFY_UPLOAD

// #define STORAGE_READ_CACHE

/* define of key generation method */
#define SGX_KEY_GEN
// #define NON_OPRF
// #define MLE
// #define SKE
// #define MinHash
// #define OPRF
// #define OPENSSL_V_1_0_2

#ifdef HIGH_SECURITY
#define CHUNK_FINGER_PRINT_SIZE 32
#define CHUNK_HASH_SIZE 32
#define CHUNK_ENCRYPT_KEY_SIZE 32
#define FILE_NAME_HASH_SIZE 32
#else
#define CHUNK_FINGER_PRINT_SIZE 16
#define CHUNK_HASH_SIZE 16
#define CHUNK_ENCRYPT_KEY_SIZE 16
#define FILE_NAME_HASH_SIZE 16
#endif

#define CHUNKER_FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define CHUNKER_VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker
#define CHUNKER_TRACE_DRIVEN_TYPE_FSL 2
#define CHUNKER_TRACE_DRIVEN_TYPE_UBC 3
#define MIN_CHUNK_SIZE 4096 //macro for the min size of variable-size chunker
#define AVG_CHUNK_SIZE 8192 //macro for the average size of variable-size chunker
#define MAX_CHUNK_SIZE 16384 //macro for the max size of variable-size chunker

#define CHUNK_FINGER_PRINT_SIZE 32
#define CHUNK_HASH_SIZE 32
#define CHUNK_ENCRYPT_KEY_SIZE 32
#define FILE_NAME_HASH_SIZE 32
#define KEY_SERVER_SESSION_KEY_SIZE 32

#define CHUNK_TYPE_ENCRYPTED 0
#define CHUNK_TYPE_VERIFY_PASSED 1
#define CHUNK_TYPE_VERIFY_NOT_PASSED 2
#define CHUNK_TYPE_SENDING_OVER 3
#define CHUNK_TYPE_UNIQUE 4
#define CHUNK_TYPE_DUPLICATE 5
#define CHUNK_TYPE_INIT 6
#define CHUNK_TYPE_NEED_UPLOAD 7

#define DATA_TYPE_RECIPE 1
#define DATA_TYPE_CHUNK 2

#define NETWORK_MESSAGE_DATA_SIZE 12 * 1000 * 1000
#define SGX_MESSAGE_MAX_SIZE 1024 * 1024
#define NETWORK_RESPOND_BUFFER_MAX_SIZE 12 * 1000 * 1000
#define CRYPTO_BLOCK_SZIE 16

class Configure {
private:
    // following settings configure by macro set
    uint64_t _runningType; // localDedup \ serverDedup

    // chunking settings
    uint64_t _chunkingType; // varSize \ fixedSize \ simple
    uint64_t _maxChunkSize;
    uint64_t _minChunkSize;
    uint64_t _averageChunkSize;
    uint64_t _slidingWinSize;
    uint64_t _segmentSize; // if exist segment function
    uint64_t _ReadSize; //128M per time

    // key management settings
    uint64_t _keyServerNumber;
    std::vector<std::string> _keyServerIP;
    std::vector<int> _keyServerPort;
    int _keyServerRArequestPort;
    int _keyBatchSize;
    uint64_t _keyCacheSize;
    uint64_t _keyGenLimitPerSessionKey;
    uint64_t _keyRegressionMaxTimes;

    //muti thread settings
    int _encodeThreadLimit;
    int _keyClientThreadLimit;
    int _keyServerThreadLimit;
    int _senderThreadLimit;
    int _recvDecodeThreadLimit;
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
    int _POWIasServerType; //0 for develop; 1 for production
    uint64_t _POWBatchSize;

    //km enclave settings
    int _KMQuoteType; //0x00 linkable; 0x01 unlinkable
    int _KMIasVersion;
    std::string _KMServerIp;
    int _KMServerPort;
    std::string _KMEnclaveName;
    std::string _KMSPID;
    int _KMIasServerType; //0 for develop; 1 for production

    // storage management settings
    uint64_t _storageServerNumber;
    std::vector<std::string> _storageServerIP;
    std::vector<int> _storageServerPort;
    uint64_t _maxContainerSize;

    //server setting
    std::string _RecipeRootPath;
    std::string _containerRootPath;
    std::string _fp2ChunkDBName;
    std::string _fp2MetaDBame;
    uint64_t _raSessionKeylifeSpan;

    //client settings
    int _clientID;
    int _sendChunkBatchSize;
    int _sendRecipeBatchSize;

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

    // key management settings
    uint64_t getKeyServerNumber();
    std::string getKeyServerIP();
    //std::vector<std::string> getkeyServerIP();
    int getKeyServerPort();
    //std::vector<int> getKeyServerPort();
    int getkeyServerRArequestPort();
    int getKeyBatchSize();
    uint64_t getKeyCacheSize();
    uint64_t getKeyGenLimitPerSessionkeySize();
    uint64_t getKeyRegressionMaxTimes();

    //muti thread settings
    int
    getEncoderThreadLimit();
    int getKeyClientThreadLimit();
    int getKeyServerThreadLimit();

    int getSenderThreadLimit();
    int getRecvDecodeThreadLimit();
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
    uint64_t getPOWBatchSize();

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
    std::string getStorageServerIP();
    //std::vector<std::string> getStorageServerIP();

    int getStorageServerPort();
    //std::vector<int> getStorageServerPort();

    uint64_t getMaxContainerSize();

    //server settings
    std::string getRecipeRootPath();
    std::string getContainerRootPath();
    std::string getFp2ChunkDBName();
    std::string getFp2MetaDBame();
    uint64_t getRASessionKeylifeSpan();

    //client settings
    int getClientID();
    int getSendChunkBatchSize();
    int getSendRecipeBatchSize();

    //timer settings
    double getTimeOutScale();
};

#endif //GENERALDEDUPSYSTEM_CONFIGURE_HPP
