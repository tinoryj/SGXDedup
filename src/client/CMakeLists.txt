add_library(_chunker STATIC _chunker.cpp)
add_library(chunker STATIC chunker.cpp)
add_library(_encoder STATIC _encoder.cpp)
add_library(encoder STATIC encoder.cpp)
add_library(keyClient STATIC keyClient.cpp)
add_library(_sender _sender.cpp)
add_library(sender sender.cpp)
add_library(_receiver _receiver.cpp)
add_library(receiver receiver.cpp)
add_library(_decoder _decoder.cpp)
add_library(decoder decoder.cpp)
add_library(_retriever _retriever.cpp)
add_library(retriever retriever.cpp)

add_executable(client clientMain.cpp)

#target_link_libraries(client configure SSL_TLS boost_thread boost_system chunker keyClient encoder cache)
target_link_libraries(client ${BOOST_LIBRARY_OBJ} ${OPENSSL_LIBRARY_OBJ}
        ${SYSTEM_LIBRARY_OBJ} ${UTIL_OBJ} ${CLIENT_OBJ})

#target_link_libraries(chunker _chunker CryptoPrimitive chunk)
target_link_libraries(chunker ${UTIL_OBJ} _chunker)

#target_link_libraries(encoder _messageQueue _encoder ssl crypto)
target_link_libraries(encoder ${OPENSSL_OBJ} ${UTIL_OBJ} _encoder)

#target_link_libraries(sender _sender configure Socket chunk _messageQueue seriazation)
target_link_libraries(sender ${UTIL_OBJ} _sender)

#
target_link_libraries(receiver ${UTIL_OBJ} _receiver)

#
target_link_libraries(retriever ${UTIL_OBJ} _retriever)

#
target_link_libraries(decoder ${UTIL_OBJ} _decoder)