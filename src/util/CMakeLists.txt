add_library(_messageQueue STATIC _messageQueue.cpp)
add_library(chunk STATIC chunk.cpp)
add_library(configure STATIC configure.cpp)
add_library(CryptoPrimitive STATIC CryptoPrimitive.cpp)
add_library(seriazation STATIC seriazation.cpp)
add_library(SSL_TLS STATIC ssl.cpp)
add_library(Sock Sock.cpp)

###
add_library(cache cache.cpp)
###

#target_link_libraries(seriazation boost_serialization chunk)
target_link_libraries(seriazation ${BOOST_LIBRARY_OBJ} chunk)

#target_link_libraries(SSL_TLS ssl)
target_link_libraries(SSL_TLS ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})

#target_link_libraries(_messageQueue pthread rt seriazation)
target_link_libraries(_messageQueue ${SYSTEM_LIBRARY_OBJ} ${BOOST_LIBRARY_OBJ} seriazation)

#target_link_libraries(CryptoPrimitive crypto)
target_link_libraries(CryptoPrimitive ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})

#target_link_libraries(cache boost_thread pthread)
target_link_libraries(cache ${BOOST_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ})