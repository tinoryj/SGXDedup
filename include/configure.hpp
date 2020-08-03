#ifndef SGXDEDUP_CONFIGURE_HPP
#define SGXDEDUP_CONFIGURE_HPP

#include <bits/stdc++.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;

/* System Test Settings: 0-disable; 1-enable */
#define SYSTEM_BREAK_DOWN 1
#define SYSTEM_DEBUG_FLAG 0
#define OPENSSL_V_1_0_2 0
#define ENCODER_MODULE_ENABLED 0 // if enable encoder, chunk encryption will move from keyclient to encoder
#define FINGERPRINTER_MODULE_ENABLE 1
#define ENCLAVE_SEALED_INIT_ENABLE 1

/* Key Generation method Settings: 0-disable; 1-enable */
#define KEY_GEN_SGX_CFB 0
#define KEY_GEN_SGX_CTR 1
#define KEY_GEN_SERVER_MLE_NO_OPRF 0

/* Storage Core Settings: 0-disable; 1-enable */
#define STORAGE_SERVER_VERIFY_UPLOAD 0
#define STORAGE_CORE_READ_CACHE 0

#define KEY_GEN_SGX_MULTITHREAD_ENCLAVE 1
#define KEY_GEN_EPOLL_MODE 0

#define KEY_REGRESSION_BY_INTERVALS 0
#define KEY_REGRESSION_BY_NUMBER 1
#define KEY_REGRESSION_METHOD KEY_REGRESSION_BY_NUMBER

/* Message Queue Settings: QUEUE_TYPE to setup the default message queue*/
#define QUEUE_TYPE_LOCKFREE_SPSC_QUEUE 0
#define QUEUE_TYPE_LOCKFREE_QUEUE 1
#define QUEUE_TYPE_READ_WRITE_QUEUE 2
#define QUEUE_TYPE_CONCURRENT_QUEUE 3
#define QUEUE_TYPE QUEUE_TYPE_LOCKFREE_SPSC_QUEUE

/* Recipe Management Settings: 0-disable; 1-enable */
#define ENCRYPT_WHOLE_RECIPE_FILE 0
#define ENCRYPT_ONLY_KEY_RECIPE_FILE 1
#define RECIPE_MANAGEMENT_METHOD ENCRYPT_WHOLE_RECIPE_FILE

/* System Running Type Settings */
#define CHUNKER_FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define CHUNKER_VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker
#define CHUNKER_TRACE_DRIVEN_TYPE_FSL 2
#define CHUNKER_TRACE_DRIVEN_TYPE_UBC 3

/* System Infomation Size Settings */
#define CHUNK_FINGER_PRINT_SIZE 32
#define CHUNK_HASH_SIZE 32
#define CHUNK_ENCRYPT_KEY_SIZE 32
#define FILE_NAME_HASH_SIZE 32

#define MIN_CHUNK_SIZE 4096 //macro for the min size of variable-size chunker
#define AVG_CHUNK_SIZE 8192 //macro for the average size of variable-size chunker
#define MAX_CHUNK_SIZE 16384 //macro for the max size of variable-size chunker

#define NETWORK_MESSAGE_DATA_SIZE 12 * 1000 * 1000
#define SGX_MESSAGE_MAX_SIZE 1024 * 1024
#define NETWORK_RESPOND_BUFFER_MAX_SIZE 12 * 1000 * 1000
#define CRYPTO_BLOCK_SZIE 16
#define KEY_SERVER_SESSION_KEY_SIZE 32

/* System Infomation Type Settings */
#define DATA_TYPE_RECIPE 1
#define DATA_TYPE_CHUNK 2

#define CHUNK_TYPE_ENCRYPTED 0
#define CHUNK_TYPE_VERIFY_PASSED 1
#define CHUNK_TYPE_VERIFY_NOT_PASSED 2
#define CHUNK_TYPE_SENDING_OVER 3
#define CHUNK_TYPE_UNIQUE 4
#define CHUNK_TYPE_DUPLICATE 5
#define CHUNK_TYPE_INIT 6
#define CHUNK_TYPE_NEED_UPLOAD 7

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
    uint64_t _keyEnclaveThreadNumber;
    std::vector<std::string> _keyServerIP;
    std::vector<int> _keyServerPort;
    int _keyServerRArequestPort;
    int _keyBatchSize;
    uint64_t _keyCacheSize;
    uint64_t _keyGenLimitPerSessionKey;
    uint32_t _keyRegressionMaxTimes;
    uint32_t _keyRegressionIntervals;

    //POW settings
    int _POWQuoteType; //0x00 linkable; 0x01 unlinkable
    int _POWIasVersion;
    int _POWServerPort;
    std::string _POWEnclaveName;
    std::string _POWSPID;
    int _POWIasServerType; //0 for develop; 1 for production
    uint64_t _POWBatchSize;
    std::string _POWPriSubscriptionKey;
    std::string _POWSecSubscriptionKey;

    //km enclave settings
    int _KMQuoteType; //0x00 linkable; 0x01 unlinkable
    int _KMIasVersion;
    int _KMServerPort;
    std::string _KMEnclaveName;
    std::string _KMSPID;
    int _KMIasServerType; //0 for develop; 1 for production
    std::string _KMPriSubscriptionKey;
    std::string _KMSecSubscriptionKey;

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

    // any additional settings

public:
    //  Configure(std::ifstream& confFile); // according to setting json to init configure class
    Configure(std::string path);

    Configure();

    ~Configure();

    void readConf(std::string path);

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
    uint64_t getKeyEnclaveThreadNumber();
    std::string getKeyServerIP();
    //std::vector<std::string> getkeyServerIP();
    int getKeyServerPort();
    //std::vector<int> getKeyServerPort();
    int getkeyServerRArequestPort();
    int getKeyBatchSize();
    uint64_t getKeyCacheSize();
    uint64_t getKeyGenLimitPerSessionkeySize();
    uint32_t getKeyRegressionMaxTimes();
    uint32_t getKeyRegressionIntervals(); // unit: sec

    //pow settings
    int getPOWQuoteType();
    int getPOWIASVersion();
    int getPOWServerPort();
    std::string getPOWEnclaveName();
    std::string getPOWSPID();
    int getPOWIASServerType();
    uint64_t getPOWBatchSize();
    std::string getPOWPriSubscriptionKey();
    std::string getPOWSecSubscriptionKey();

    //km settings
    int getKMQuoteType();
    int getKMIASVersion();
    int getKMServerPort();
    std::string getKMEnclaveName();
    std::string getKMSPID();
    int getKMIASServerType();
    std::string getKMPriSubscriptionKey();
    std::string getKMSecSubscriptionKey();

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
};

#endif //SGXDEDUP_CONFIGURE_HPP
