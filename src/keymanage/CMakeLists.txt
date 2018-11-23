add_library(_keyManager STATIC _keyManager.cpp)
add_library(keyServer STATIC keyServer.cpp)

add_executable(keymanager keyServerMain.cpp)

#target_link_libraries(keyServer configure SSL_TLS _keyManager cache)
target_link_libraries(keyServer ${OPENSSL_LIBRARY_OBJ} ${UTIL_OBJ} _keyManager)

#target_link_libraries(keymanager keyServer)
target_link_libraries(keymanager keyServer)