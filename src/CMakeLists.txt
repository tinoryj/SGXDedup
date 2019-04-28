#/

#find_package(Boost 1.36.0 REQUIRED COMPONENTS thread system serialization)
#if(Boost_FOUND)
  set(Boost_INCLUDE_DIRS /home/public/include)
  set(BOOST_LIBRARY_DIR /home/public/lib)
  include_directories(${Boost_INCLUDE_DIRS})
# link_directories(${Boost_LIBRARIES})
  link_directories(${BOOST_LIBRARY_DIR})
#endif()


set(OPENSSL_INCLUDE_PATH /usr/include/)
set(OPENSSL_LIBRARY_PATH /usr/lib/x86_64-linux-gnu)

set(LEVELDB_INCLUDE_PATH /home/public/lib/leveldb/include)
set(LEVELDB_LIBRARY_PATH /home/public/lib/leveldb/lib)
#set(LEVELDB_INCLUDE_PATH ../lib/leveldb/include)
#set(LEVELDB_LIBRARY_PATH ../lib/leveldb/out-shared)

set(SGX_INCLUDE_PATH /opt/intel/sgxsdk/include)

include_directories(${SGX_INCLUDE_PATH})
include_directories(${OPENSSL_INCLUDE_PATH} ${LEVELDB_INCLUDE_PATH} ../include)
link_directories(${OPENSSL_LIBRARY_PATH} ${LEVELDB_LIBRARY_PATH} util)
set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)
set(LEVELDB_LIBRARY_OBJ pthread leveldb)
set(UTIL_OBJ _messageQueue chunk configure CryptoPrimitive SSL_TLS Sock database cache)

set(LINK_OBJ ${UTIL_OBJ} ${OPENSSL_LIBRARY_OBJ} ${LEVELDB_LIBRARY_OBJ}
        ${BOOST_LIBRARY_OBJ} ${UTIL_OBJ} ${SYSTEM_LIBRARY_OBJ})

set(CLIENT_OBJ chunker _chunker encoder _encoder keyClient sender _sender receiver _receiver
        decoder _decoder retriever _retriever powClient)
set(SERVER_OBJ dataSR _dataSR dedupCore _dedupCore storage _storage powServer)
set(KEYSERVER_OBJ keyServer _keyManager)

#-fno-builtin-memset
set(CMAKE_C_FLAGS "-O3 -g")
set(CMAKE_CXX_FLAGS "-O3 -g -std=c++11")
set(CMAKE_MODULE_PATH /usr/share/cmake-3.10/Modules/)
list(APPEND CMAKE_MODULE_PATH ${PATH_TO_FindSGX.cmake_FILE})
find_package(SGX REQUIRED)

add_subdirectory(pow)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(keymanage)
add_subdirectory(util)