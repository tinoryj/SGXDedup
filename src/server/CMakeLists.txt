add_library(_dataSR _dataSR.cpp)
add_library(_storage _storage.cpp)
add_library(_dedupCore _dedupCore.cpp)

add_executable(server ServerMain.cpp)

target_link_libraries(server _dataSR _storage _dedupCore)