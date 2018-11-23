add_library(_chunker STATIC _chunker.cpp)
add_library(chunker STATIC chunker.cpp)
add_library(_encoder STATIC _encoder.cpp)
add_library(encoder STATIC encoder.cpp)
add_library(keyClient STATIC keyClient.cpp)

set(LINK_OBJ _chunker _encoder encoder keyClient chunker)

add_executable(client clientMain.cpp)

#target_link_libraries(client configure SSL_TLS boost_thread boost_system chunker keyClient encoder cache)
target_link_libraries(client ${BOOST_LIBRARY_OBJ} ${OPENSSL_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${UTIL_OBJ} ${CLIENT_OBJ})

#target_link_libraries(chunker _chunker CryptoPrimitive chunk)
target_link_libraries(chunker ${UTIL_OBJ} _chunker)

#target_link_libraries(encoder _messageQueue _encoder ssl crypto)
target_link_libraries(encoder ${OPENSSL_OBJ} ${UTIL_OBJ} _encoder)