#src/Client-Simulator

add_library(sender STATIC sender.cpp)
add_executable(client-simulator clientSimulatorMain.cpp)

target_link_libraries(client-simulator ${CLIENT_OBJ} ${LINK_OBJ})