#src/pow/src

#add_library(powClient PowClient.cpp)
#target_link_libraries(powClient enclave_untrusted ${SGX_U_LIBRARY_OBJ} sender)

add_library(powServer PowServer.cpp)
add_library(kmServer km_server.cpp)

target_link_libraries(powServer base64 sgx_crypto iasrequest hexutil byteorder agent_wget POW_json )
target_link_libraries(kmServer base64 sgx_crypto iasrequest hexutil byteorder agent_wget POW_json)