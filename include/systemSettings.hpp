#ifndef SGXDEDUP_SYSTEMSETTINGS_HPP
#define SGXDEDUP_SYSTEMSETTINGS_HPP
/* System Test Settings: 0-disable; 1-enable */
#define SYSTEM_BREAK_DOWN 0 // Compute and output system modules process time infomation
#define SYSTEM_DEBUG_FLAG 0
 // Debug flag for modify and output each step's content in system
#define OPENSSL_V_1_0_2 0 // Compat to old version openssl, which has different API with OpenSSL 1.1 series
#define ENCLAVE_SEALED_INIT_ENABLE 1 // Whether to use the client pow enclave local startup function (if closed, remote authentication is required for each startup) 

/* Key Generation method Settings */
#define KEY_GEN_SGX_CFB 0
#define KEY_GEN_SGX_CTR 1
#define KEY_GEN_METHOD_TYPE KEY_GEN_SGX_CTR // The system key generate method setting

/* System Infomation Type Settings (For internal use, please do not modify) */
// System input types
#define CHUNKER_FIX_SIZE_TYPE 0 //macro for the type of fixed-size chunker
#define CHUNKER_VAR_SIZE_TYPE 1 //macro for the type of variable-size chunker
#define CHUNKER_TRACE_DRIVEN_TYPE_FSL 2 //macro for the type of fsl dataset chunk generator
#define CHUNKER_TRACE_DRIVEN_TYPE_MS 3 //macro for the type of ms dataset chunk generator
// Data type identification
#define DATA_TYPE_RECIPE 1
#define DATA_TYPE_CHUNK 2
#define CHUNK_TYPE_NEED_UPLOAD 9
// System Infomation Size Settings, for memory allocation
#define CHUNK_HASH_SIZE 32 // SHA256 hash size
#define CHUNK_ENCRYPT_KEY_SIZE 32 // AES256 key size
#define FILE_NAME_HASH_SIZE 32 // SHA256 hash size
#define MAX_CHUNK_SIZE 16384 //max size of chunker content
#define NETWORK_MESSAGE_DATA_SIZE 18 * 1000 * 1000 // general message buffer for network transmit
#define SGX_MESSAGE_MAX_SIZE 1024 * 1024 // sgx message buffer for network transmit
#define CRYPTO_BLOCK_SZIE 16 // AES256 crypto unit
#define KEY_SERVER_SESSION_KEY_SIZE 32 // Generated key server session key size
// System test flags
#define MULTI_CLIENT_UPLOAD_TEST_MODE 0
#define POW_TEST_MODE 0

#endif //SGXDEDUP_SYSTEMSETTINGS_HPP