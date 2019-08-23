#src/client

set(OPENSSL_INCLUDE_PATH /usr/local/openssl1.1.1/include)
set(OPENSSL_LIBRARY_PATH /usr/local/openssl1.1.1/lib)

add_library(chunker STATIC chunker.cpp)
add_library(keyClient STATIC keyClient.cpp)
add_library(sender STATIC sender.cpp)
add_library(recvDecode STATIC recvDecode.cpp)
add_library(retriever STATIC retriever.cpp)

add_executable(client clientMain.cpp)

target_link_libraries(client ${CLIENT_OBJ} ${LINK_OBJ} hexutil)