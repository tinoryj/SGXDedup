#src/enclave/src

add_library(powServer powServer.cpp)
add_library(kmServer kmServer.cpp)


target_link_libraries(powServer base64 sgx_crypto iasrequest hexutil byteorder agent_wget sgx_json sgxErrorSupport common logfile)
target_link_libraries(kmServer base64 sgx_crypto iasrequest hexutil byteorder agent_wget sgx_json sgxErrorSupport common logfile)
