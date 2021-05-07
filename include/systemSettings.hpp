#ifndef SGXDEDUP_SYSTEMSETTINGS_HPP
#define SGXDEDUP_SYSTEMSETTINGS_HPP
/* System Test Settings: 0-disable; 1-enable */
#define SYSTEM_BREAK_DOWN 1
#define SYSTEM_DEBUG_FLAG 0
#define OPENSSL_V_1_0_2 0
#define ENCODER_MODULE_ENABLED 0 // if enable encoder, chunk encryption will move from keyclient to encoder
#define FINGERPRINTER_MODULE_ENABLE 1
#define ENCLAVE_SEALED_INIT_ENABLE 1
#define MULTI_CLIENT_UPLOAD_TEST 0
#define TRACE_DRIVEN_TEST 0
#define POW_TEST 1

/* Key Generation method Settings: 0-disable; 1-enable */
#define KEY_GEN_SGX_CFB 0
#define KEY_GEN_SGX_CTR 1
#define KEY_GEN_SERVER_MLE_NO_OPRF 2
#define KEY_GEN_METHOD_TYPE KEY_GEN_SGX_CTR

/* Storage Core Settings: 0-disable; 1-enable */
#define STORAGE_SERVER_VERIFY_UPLOAD 0
#define STORAGE_CORE_READ_CACHE 0

#define KEY_GEN_SGX_MULTITHREAD_ENCLAVE 1

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
#define CHUNKER_TRACE_DRIVEN_TYPE_FSL 2 //macro for the type of fsl dataset chunk generator
#define CHUNKER_TRACE_DRIVEN_TYPE_UBC 3 //macro for the type of ms dataset chunk generator

/* System Infomation Size Settings */
#define CHUNK_HASH_SIZE 32
#define CHUNK_ENCRYPT_KEY_SIZE 32
#define FILE_NAME_HASH_SIZE 32

#define MIN_CHUNK_SIZE 4096 //macro for the min size of variable-size chunker
#define AVG_CHUNK_SIZE 8192 //macro for the average size of variable-size chunker
#define MAX_CHUNK_SIZE 16384 //macro for the max size of variable-size chunker

#if TRACE_DRIVEN_TEST == 1
#define NETWORK_MESSAGE_DATA_SIZE 18 * 1000 * 1000
#else
#define NETWORK_MESSAGE_DATA_SIZE 12 * 1000 * 1000
#endif
#define SGX_MESSAGE_MAX_SIZE 1024 * 1024
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

#endif //SGXDEDUP_SYSTEMSETTINGS_HPP