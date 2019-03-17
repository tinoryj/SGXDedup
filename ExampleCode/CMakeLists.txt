#ExampleCode

set(BOOST_INCLUDE_DIR /home/public/include)
set(BOOST_LIBRARY_DIR /home/public/lib)
set(OPENSSL_INCLUDE_PATH /usr/include/)
set(OPENSSL_LIBRARY_PATH /usr/lib/x86_64-linux-gnu)
set(LEVELDB_INCLUDE_PATH /home/public/lib/leveldb/include)
set(LEVELDB_LIBRARY_PATH /home/public/lib/leveldb/lib)

set(UTIL_OBJ SSL _messageQueue chunk configure CryptoPrimitive serialization)
set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)
set(LEVELDB_LIBRARY_OBJ pthread leveldb)

set(CMAKE_C_FLAGS -g)
set(CMAKE_CXX_FLAGS -g)

include_directories(${BOOST_INCLUDE_DIR} ${OPENSSL_INCLUDE_PATH} ${LEVELDB_INCLUDE_PATH} include)
link_directories(${BOOST_LIBRARY_DIR} ${OPENSSL_LIBRARY_PATH} ${LEVELDB_LIBRARY_PATH} util)


add_executable(tls_server TLS_SSL_Server.cpp)
add_executable(tls_client TLS_SSL_Client.cpp)
add_executable(epoll_client epoll_client.cpp)
add_executable(epoll_server epoll_server.cpp)
add_executable(tls_epoll_client tls_epoll_client.cpp)
add_executable(tls_epoll_server tls_epoll_server.cpp)
add_executable(boostthread boost_thread.cpp)
add_executable(boostjson boost_json.cpp)
#add_executable(level leveldb.cpp)
#add_executable(Boost_bind Boost_bind.cpp)
add_executable(Boost_seriazation Boost_seriazation.cpp)
#add_executable(message_queue message_queue.cpp)
add_executable(opensslbn RSAsign.cpp)
add_executable(lru_cache Boost_lru.cpp)
add_executable(openssltest RSASIGN.cpp)
add_executable(mq message_queue.cpp)
add_executable(chrono chronoTime.cpp)

add_executable(opensslkey opensslkey.cpp)

target_link_libraries(tls_server ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})
target_link_libraries(tls_client ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})

target_link_libraries(tls_epoll_client ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})
target_link_libraries(tls_epoll_server ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(boostthread ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(boostjson ${BOOST_LIBRARY_OBJ})
#target_link_libraries(level ${LEVELDB_LIBRARY_OBJ})

target_link_libraries(Boost_seriazation ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(opensslbn ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(lru_cache ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(openssltest ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(opensslkey ${OPENSSL_LIBRARY_OBJ})

target_link_libraries(mq ${BOOST_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})