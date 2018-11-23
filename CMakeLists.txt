cmake_minimum_required(VERSION 3.9)
project(generaldedupsystem)

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH ${generaldedupsystem_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${generaldedupsystem_SOURCE_DIR}/lib)

add_subdirectory(src)

add_subdirectory(ExampleCode)