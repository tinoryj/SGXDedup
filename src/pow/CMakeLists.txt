
include_directories(include)

add_subdirectory(enclave)
add_subdirectory(powapp)
add_subdirectory(util)
add_subdirectory(attestationServer)

add_library(tet test.cpp)