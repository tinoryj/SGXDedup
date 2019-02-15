add_library(powClient PowClient.cpp)
target_link_libraries(powClient enclave_untrusted ${SGX_U_LIBRARY_OBJ})

add_library(powServer PowServer.cpp)

target_link_libraries(powServer base64 sgx_crypto iasrequest hexutil byteorder agent_wget POW_json)