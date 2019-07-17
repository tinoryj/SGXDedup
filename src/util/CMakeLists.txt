#src/util

add_library(configure STATIC configure.cpp)
add_library(CryptoPrimitive STATIC CryptoPrimitive.cpp)
add_library(SSL_TLS STATIC ssl.cpp)
add_library(Sock STATIC Socket.cpp)

add_library(cache STATIC cache.cpp)
add_library(database STATIC database.cpp)


