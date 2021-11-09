#/
set(CMAKE_MODULE_PATH /usr/share/cmake-3.16/Modules/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "../CMakeModules/")
# list(APPEND CMAKE_MODULE_PATH ${PATH_TO_FindSGX.cmake_FILE})
find_package(SGX REQUIRED sgx_tstdc sgx_tcxx sgx_pthread sgx_tcrypto sgx_tsgxssl sgx_tsgxssl_crypto sgx_usgxssl sgx_uae_launch sgx_uae_quote_ex sgx_uae_epid sgx_uae_service)
set(SGXOPENSSL_INCLUDE_PATH /opt/intel/sgxssl/include)
set(SGXOPENSSL_LIBRARY_PATH /opt/intel/sgxssl/lib64)
find_package(Boost REQUIRED COMPONENTS thread system serialization)
if(Boost_FOUND)
   include_directories(${BOOST_INCLUDE_DIRS})
   link_directories(${BOOST_LIBRARY_DIR})
endif()


set(SGX_INCLUDE_PATH /opt/intel/sgxsdk/include)
set(ENCLAVE_INCLUDE_PATH enclave/include)

include_directories(${SGX_INCLUDE_PATH} ${ENCLAVE_INCLUDE_PATH} ../include)
link_directories(util)

set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)
set(LEVELDB_LIBRARY_OBJ pthread leveldb)
set(SGX_SDK_LIBRARY_OBJ sgx_uae_launch sgx_uae_quote_ex sgx_uae_epid sgx_uae_service)
set(UTIL_OBJ configure cryptoPrimitive SSL_TLS database)
set(ENCLAVE_UTILS_OBJ common logfile agent_wget base64 byteorder sgx_crypto sgx_json hexutil sgxErrorSupport iasrequest)

set(LINK_OBJ ${UTIL_OBJ} ${OPENSSL_LIBRARY_OBJ} ${LEVELDB_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})

#-fno-builtin-memset

set (CMAKE_SYSTEM_NAME Linux )
if (NOT CMAKE_BUILD_TYPE)
   set (CMAKE_BUILD_TYPE "Release")
   message (
      STATUS "No CMAKE_BUILD_TYPE selected, defaulting to ${CMAKE_BUILD_TYPE}"
   )
endif ()

set(CMAKE_C_FLAGS_DEBUG "-Wall -std=c1x -g -ggdb -g2") 
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++17 -g -ggdb -g2 -fno-omit-frame-pointer -fsanitize=leak")
set(CMAKE_C_FLAGS_RELEASE "-O3 -std=c1x -g") 
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -std=c++17 -g")

SET (CMAKE_C_COMPILER             "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
SET (CMAKE_AR      "/usr/bin/llvm-ar")
SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
SET (CMAKE_NM      "/usr/bin/llvm-nm")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")

add_subdirectory(enclave)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(keymanage)
add_subdirectory(util)