add_library(_dataSR _dataSR.cpp)
add_library(_storage _storage.cpp)
add_library(_dedupCore _dedupCore.cpp)
add_library(dataSR dataSR.cpp)
add_library(dedupCore dedupCore.cpp)
add_library(storage storageCore.cpp)


target_link_libraries(_dataSR ${UTIL_OBJ} ${CLIENT_OBJ} ${BOOST_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${OPENSSL_LIBRARY_OBJ} ${LEVELDB_LIBRARY_OBJ})
target_link_libraries(_storage ${UTIL_OBJ} ${CLIENT_OBJ} ${BOOST_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${OPENSSL_LIBRARY_OBJ} ${LEVELDB_LIBRARY_OBJ})
target_link_libraries(_dedupCore ${UTIL_OBJ} ${CLIENT_OBJ} ${BOOST_LIBRARY_OBJ} ${SYSTEM_LIBRARY_OBJ} ${OPENSSL_LIBRARY_OBJ} ${LEVELDB_LIBRARY_OBJ})

target_link_libraries(dataSR _dataSR)
target_link_libraries(dedupCore _dedupCore)
target_link_libraries(storage _storage)

add_executable(server ServerMain.cpp)
target_link_libraries(server dataSR storage dedupCore powServer)
