#src/util

add_library(configure STATIC configure.cpp)
add_library(cryptoPrimitive STATIC cryptoPrimitive.cpp)
add_library(Sock STATIC socket.cpp)
add_library(SSL_TLS STATIC ssl.cpp)
add_library(database STATIC database.cpp)
#add_library(messageQueue STATIC messageQueue.cpp)


