# src/enclave/km_enclave

set(EDL_SEARCH_PATHS ${SGXOPENSSL_INCLUDE_PATH} /opt/intel/sgxsdk/include)

set(E_SRC km_enclave.cpp)
set(LDS km_enclave.lds)
add_enclave_library(km_enclave
                    USE_SGXSSL
                    SRCS ${E_SRC}
                    EDL km_enclave.edl
                    EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS}
                    LDSCRIPT ${LDS})

target_link_libraries(km_enclave)

enclave_sign(km_enclave KEY km_enclave_private.pem CONFIG km_enclave.config.xml)

set(U_SRC ../src/kmClient.cpp)
add_untrusted_library(kmClient
                     STATIC
                     SRCS ${U_SRC}
                     EDL km_enclave.edl
                     EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})
