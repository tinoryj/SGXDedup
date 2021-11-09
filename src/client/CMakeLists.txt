#src/client

set(SGX_OPENSSL_LIBRARY_PATH /opt/intel/sgxssl/lib64)
set(SGX_OPENSSL_OBJ sgx_usgxssl)
link_directories(${SGX_OPENSSL_LIBRARY_PATH})
link_directories(${SGX_SDK_LIBRARY_PATH})

add_library(chunker STATIC chunker.cpp)
add_library(fingerprinter STATIC fingerprinter.cpp)
add_library(keyClient STATIC keyClient.cpp)
add_library(encoder STATIC encoder.cpp)
add_library(sender STATIC sender.cpp)
add_library(recvDecode STATIC recvDecode.cpp)
add_library(retriever STATIC retriever.cpp)

set(CLIENT_OBJ chunker fingerprinter keyClient encoder powClient sender recvDecode  retriever)

add_executable(client-sgx clientMain.cpp)

target_link_libraries(client-sgx ${CLIENT_OBJ} ${LINK_OBJ} ${SGX_OPENSSL_OBJ} hexutil sgxErrorSupport)
