# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tinoryj/code/generaldedupsystem

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tinoryj/code/generaldedupsystem/build

# Include any dependencies generated for this target.
include ExampleCode/CMakeFiles/tls_server.dir/depend.make

# Include the progress variables for this target.
include ExampleCode/CMakeFiles/tls_server.dir/progress.make

# Include the compile flags for this target's objects.
include ExampleCode/CMakeFiles/tls_server.dir/flags.make

ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o: ExampleCode/CMakeFiles/tls_server.dir/flags.make
ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o: ../ExampleCode/TLS_SSL_Server.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o -c /home/tinoryj/code/generaldedupsystem/ExampleCode/TLS_SSL_Server.cpp

ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.i"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tinoryj/code/generaldedupsystem/ExampleCode/TLS_SSL_Server.cpp > CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.i

ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.s"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tinoryj/code/generaldedupsystem/ExampleCode/TLS_SSL_Server.cpp -o CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.s

ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.requires:

.PHONY : ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.requires

ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.provides: ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.requires
	$(MAKE) -f ExampleCode/CMakeFiles/tls_server.dir/build.make ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.provides.build
.PHONY : ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.provides

ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.provides.build: ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o


# Object files for target tls_server
tls_server_OBJECTS = \
"CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o"

# External object files for target tls_server
tls_server_EXTERNAL_OBJECTS =

../bin/tls_server: ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o
../bin/tls_server: ExampleCode/CMakeFiles/tls_server.dir/build.make
../bin/tls_server: ExampleCode/CMakeFiles/tls_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/tls_server"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tls_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ExampleCode/CMakeFiles/tls_server.dir/build: ../bin/tls_server

.PHONY : ExampleCode/CMakeFiles/tls_server.dir/build

ExampleCode/CMakeFiles/tls_server.dir/requires: ExampleCode/CMakeFiles/tls_server.dir/TLS_SSL_Server.cpp.o.requires

.PHONY : ExampleCode/CMakeFiles/tls_server.dir/requires

ExampleCode/CMakeFiles/tls_server.dir/clean:
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && $(CMAKE_COMMAND) -P CMakeFiles/tls_server.dir/cmake_clean.cmake
.PHONY : ExampleCode/CMakeFiles/tls_server.dir/clean

ExampleCode/CMakeFiles/tls_server.dir/depend:
	cd /home/tinoryj/code/generaldedupsystem/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tinoryj/code/generaldedupsystem /home/tinoryj/code/generaldedupsystem/ExampleCode /home/tinoryj/code/generaldedupsystem/build /home/tinoryj/code/generaldedupsystem/build/ExampleCode /home/tinoryj/code/generaldedupsystem/build/ExampleCode/CMakeFiles/tls_server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ExampleCode/CMakeFiles/tls_server.dir/depend

