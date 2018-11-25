set(BOOST_INCLUDE_DIR /home/public/include)
set(BOOST_LIBRARY_DIR /home/public/lib)
set(OPENSSL_INCLUDE_PATH /home/public/opt/openssl/include)
set(OPENSSL_LIBRARY_PATH /home/public/opt/openssl/lib)

set(UTIL_OBJ SSL _messageQueue chunk configure CryptoPrimitive serialization)
set(BOOST_LIBRARY_OBJ boost_thread boost_system boost_serialization)
set(SYSTEM_LIBRARY_OBJ pthread rt dl)
set(OPENSSL_LIBRARY_OBJ ssl crypto)

include_directories(${BOOST_INCLUDE_DIR} ${OPENSSL_INCLUDE_PATH} include)
link_directories(${BOOST_LIBRARY_DIR} ${OPENSSL_LIBRARY_PATH} util)


add_executable(tls_server TLS_SSL_Server.cpp)
add_executable(tls_client TLS_SSL_Client.cpp)
add_executable(epoll_client epoll_client.cpp)
add_executable(epoll_server epoll_server.cpp)
add_executable(tls_epoll_client tls_epoll_client.cpp)
add_executable(tls_epoll_server tls_epoll_server.cpp)
add_executable(boostthread boost_thread.cpp)
add_executable(boostjson boost_json.cpp)
#add_executable(Boost_bind Boost_bind.cpp)
#add_executable(Boost_seriazation Boost_seriazation.cpp)
#add_executable(message_queue message_queue.cpp)

target_link_libraries(tls_server ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})
target_link_libraries(tls_client ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})

target_link_libraries(tls_epoll_client ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})
target_link_libraries(tls_epoll_server ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(boostthread ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ})

target_link_libraries(boostjson ${BOOST_LIBRARY_OBJ})
