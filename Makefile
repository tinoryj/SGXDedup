PROJECT_DIRECTORY = /home/a/workspace/generaldedupsystem

SGX_SDK = /opt/intel/sgxsdk
SGX_LIBRARY_PATH = $(SGX_SDK)/lib64
SGX_ENCLAVE_SIGNER = $(SGX_SDK)/bin/x64/sgx_sign
SGX_EDGER8R = $(SGX_SDK)/bin/x64/sgx_edger8r

BUILDOBJ = server client keymanage enclave.sign.so

BOOST_LIBRARY_PATH = /home/public/lib/
BOOST_INCLUDE_PATH = /home/public/include/

OPENSSL_LIBRARY_PATH = /usr/lib/x86_64-linux-gnu
OPENSSL_INCLUDE_PATH = /usr/include/

LEVELDB_LIBRARY_PATH = /home/public/lib/leveldb/lib
LEVELDB_INCLUDE_PATH = /home/public/lib/leveldb/include

SERVER_C_FLAG = "-g"
CLEINT_C_FLAG = "-g"
KEYMANAGE_C_FLAG = "-g"

UTIL_FILE = $(wildcard $(PROJECT_DIRECTORY)/src/util/*.cpp)
UTIL_OBJ = $(UTIL_FILE:.cpp=.o)