set(BOOST_INCLUDE_DIR /home/public/include)
set(BOOST_LIBRARY_DIR /home/public/lib)

set(OPENSSL_INCLUDE_PATH /home/public/opt/openssl/include)
set(OPENSSL_LIBRARY_PATH /home/public/opt/openssl/lib)

set(LEVELDB_INCLUDE_PATH /home/public/lib/leveldb/include)
set(LEVELDB_LIBRARY_PATH /home/public/lib/leveldb/lib)

set(UTIL_OBJ SSL _messageQueue chunk configure CryptoPrimitive seriazation)
set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)
set(LEVELDB_LIBRARY_OBJ pthread leveldb)

include_directories(${BOOST_INCLUDE_DIR} ${OPENSSL_INCLUDE_PATH} ${LEVELDB_INCLUDE_PATH} ../include)
link_directories(${BOOST_LIBRARY_DIR} ${OPENSSL_LIBRARY_PATH} ${LEVELDB_LIBRARY_PATH} util)

set(UTIL_OBJ _messageQueue chunk configure CryptoPrimitive seriazation SSL_TLS cache)
set(CLIENT_OBJ _chunker chunker _encoder encoder keyClient)

set(CMAKE_C_FLAGS -g)
set(CMAKE_CXX_FLAGS -g)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(keymanage)
add_subdirectory(util)