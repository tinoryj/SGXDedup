# src/pow/enclave

set(EDL_SEARCH_PATHS /opt/intel/sgxsdk/include .)
set(E_SRC isv_enclave.c)
set(LDS enclave.lds)

add_enclave_library(enclave
                    SRCS ${E_SRC}
                    EDL enclave.edl
                    EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS}
                    LDSCRIPT ${LDS})

enclave_sign(enclave KEY Enclave_private.pem CONFIG enclave.config.xml)


set(U_SRC ../src/PowClient.cpp)
add_untrusted_library(powClient
                     STATIC
                     SRCS ${U_SRC}
                     EDL enclave.edl
                     EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})