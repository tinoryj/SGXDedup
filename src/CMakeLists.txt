#/

find_package(Boost 1.36.0 REQUIRED COMPONENTS thread system serialization)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${BOOST_LIBRARY_DIR})
endif()

set(OPENSSL_INCLUDE_PATH /usr/include/)
set(OPENSSL_LIBRARY_PATH /usr/lib/x86_64-linux-gnu)

set(LEVELDB_INCLUDE_PATH ../lib/leveldb/include)
set(LEVELDB_LIBRARY_PATH ../lib/leveldb/)

set(SGX_INCLUDE_PATH /opt/intel/sgxsdk/include)
set(POW_INCLUDE_PATH pow/include)

include_directories(${SGX_INCLUDE_PATH} ${POW_INCLUDE_PATH})
include_directories(${ENCLAVE_INCLUDE_PATH} ${BOOST_INCLUDE_DIR} ${LEVELDB_INCLUDE_PATH} ../include)
link_directories(${BOOST_LIBRARY_DIR} ${LEVELDB_LIBRARY_PATH} util)

set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)
set(LEVELDB_LIBRARY_OBJ pthread leveldb)
set(UTIL_OBJ configure cryptoPrimitive SSL_TLS Sock database cache)

set(LINK_OBJ ${UTIL_OBJ} ${OPENSSL_LIBRARY_OBJ} ${LEVELDB_LIBRARY_OBJ}
        ${BOOST_LIBRARY_OBJ} ${UTIL_OBJ} ${SYSTEM_LIBRARY_OBJ})

set(CLIENT_OBJ chunker keyClient sender  recvDecode  retriever  powClient kmServer)
set(SERVER_OBJ dataSR  dedupCore powServer storage) 
set(KEYSERVER_OBJ keyServer kmClient)

#-fno-builtin-memset
# set(CMAKE_C_FLAGS "-g ") 
# set(CMAKE_CXX_FLAGS "-g -fsanitize=address")
set(CMAKE_MODULE_PATH /usr/share/cmake-3.10/Modules/)
list(APPEND CMAKE_MODULE_PATH ${PATH_TO_FindSGX.cmake_FILE})
find_package(SGX REQUIRED)

set (CMAKE_SYSTEM_NAME Linux )
SET (CMAKE_C_COMPILER             "/usr/bin/clang")
# SET (CMAKE_C_FLAGS                "-Wall -std=c1x -g")
SET (CMAKE_C_FLAGS                "-O3 -g -std=c1x")
SET (CMAKE_C_FLAGS_DEBUG          "-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
# SET (CMAKE_CXX_FLAGS                "-O3 -std=c++17 -g -ggdb -fsanitize=address")
SET (CMAKE_CXX_FLAGS                "-O3 -g -std=c++17")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
SET (CMAKE_AR      "/usr/bin/llvm-ar")
SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
SET (CMAKE_NM      "/usr/bin/llvm-nm")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")

add_subdirectory(pow)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(keymanage)
add_subdirectory(util)