#src/server
add_library(dataSR STATIC dataSR.cpp)
add_library(dedupCore STATIC dedupCore.cpp)
add_library(storage STATIC storageCore.cpp)

add_executable(server ServerMain.cpp)
target_link_libraries(server ${SERVER_OBJ} ${LINK_OBJ})
