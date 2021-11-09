# src/enclave/pow_enclave

set(EDL_SEARCH_PATHS ${SGXOPENSSL_INCLUDE_PATH} /opt/intel/sgxsdk/include)

set(E_SRC pow_enclave.cpp)
set(LDS pow_enclave.lds)
add_enclave_library(pow_enclave
                    USE_SGXSSL
                    SRCS ${E_SRC}
                    EDL pow_enclave.edl
                    EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS}
                    LDSCRIPT ${LDS})

target_link_libraries(pow_enclave)

enclave_sign(pow_enclave KEY pow_enclave_private.pem CONFIG pow_enclave.config.xml)

set(U_SRC ../src/powClient.cpp)
add_untrusted_library(powClient
                     STATIC
                     SRCS ${U_SRC}
                     EDL pow_enclave.edl
                     EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})