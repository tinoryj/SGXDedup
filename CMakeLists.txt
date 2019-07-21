cmake_minimum_required(VERSION 3.1)
project(generaldedupsystem)

set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${generaldedupsystem_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${generaldedupsystem_SOURCE_DIR}/lib)

add_subdirectory(src)