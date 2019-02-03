set(BOOST_INCLUDE_DIR /home/public/include)
set(BOOST_LIBRARY_DIR /home/public/lib)

#set(OPENSSL_INCLUDE_PATH /home/public/opt/openssl/include)
#set(OPENSSL_LIBRARY_PATH /home/public/opt/openssl/lib)

set(OPENSSL_INCLUDE_PATH /usr/include/)
set(OPENSSL_LIBRARY_PATH /usr/lib/x86_64-linux-gnu)

set(LEVELDB_INCLUDE_PATH /home/public/lib/leveldb/include)
set(LEVELDB_LIBRARY_PATH /home/public/lib/leveldb/lib)

set(SGX_INCLUDE_PATH /opt/intel/sgxsdk/include)

set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)
set(LEVELDB_LIBRARY_OBJ pthread leveldb)

include_directories(${BOOST_INCLUDE_DIR} ${OPENSSL_INCLUDE_PATH} ${LEVELDB_INCLUDE_PATH} ${SGX_INCLUDE_PATH} ../include)
link_directories(${BOOST_LIBRARY_DIR} ${OPENSSL_LIBRARY_PATH} ${LEVELDB_LIBRARY_PATH} util)

set(UTIL_OBJ _messageQueue chunk configure CryptoPrimitive SSL_TLS Sock database cache)
set(CLIENT_OBJ chunker encoder keyClient sender receiver decoder retriever)

set(CMAKE_C_FLAGS -g)
set(CMAKE_CXX_FLAGS -g)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(keymanage)
add_subdirectory(util)
#add_subdirectory(pow)