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
include src/client/CMakeFiles/sender.dir/depend.make

# Include the progress variables for this target.
include src/client/CMakeFiles/sender.dir/progress.make

# Include the compile flags for this target's objects.
include src/client/CMakeFiles/sender.dir/flags.make

src/client/CMakeFiles/sender.dir/sender.cpp.o: src/client/CMakeFiles/sender.dir/flags.make
src/client/CMakeFiles/sender.dir/sender.cpp.o: ../src/client/sender.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/client/CMakeFiles/sender.dir/sender.cpp.o"
	cd /home/tinoryj/code/generaldedupsystem/build/src/client && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/sender.dir/sender.cpp.o -c /home/tinoryj/code/generaldedupsystem/src/client/sender.cpp

src/client/CMakeFiles/sender.dir/sender.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/sender.dir/sender.cpp.i"
	cd /home/tinoryj/code/generaldedupsystem/build/src/client && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tinoryj/code/generaldedupsystem/src/client/sender.cpp > CMakeFiles/sender.dir/sender.cpp.i

src/client/CMakeFiles/sender.dir/sender.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/sender.dir/sender.cpp.s"
	cd /home/tinoryj/code/generaldedupsystem/build/src/client && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tinoryj/code/generaldedupsystem/src/client/sender.cpp -o CMakeFiles/sender.dir/sender.cpp.s

src/client/CMakeFiles/sender.dir/sender.cpp.o.requires:

.PHONY : src/client/CMakeFiles/sender.dir/sender.cpp.o.requires

src/client/CMakeFiles/sender.dir/sender.cpp.o.provides: src/client/CMakeFiles/sender.dir/sender.cpp.o.requires
	$(MAKE) -f src/client/CMakeFiles/sender.dir/build.make src/client/CMakeFiles/sender.dir/sender.cpp.o.provides.build
.PHONY : src/client/CMakeFiles/sender.dir/sender.cpp.o.provides

src/client/CMakeFiles/sender.dir/sender.cpp.o.provides.build: src/client/CMakeFiles/sender.dir/sender.cpp.o


# Object files for target sender
sender_OBJECTS = \
"CMakeFiles/sender.dir/sender.cpp.o"

# External object files for target sender
sender_EXTERNAL_OBJECTS =

../lib/libsender.a: src/client/CMakeFiles/sender.dir/sender.cpp.o
../lib/libsender.a: src/client/CMakeFiles/sender.dir/build.make
../lib/libsender.a: src/client/CMakeFiles/sender.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../../lib/libsender.a"
	cd /home/tinoryj/code/generaldedupsystem/build/src/client && $(CMAKE_COMMAND) -P CMakeFiles/sender.dir/cmake_clean_target.cmake
	cd /home/tinoryj/code/generaldedupsystem/build/src/client && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sender.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/client/CMakeFiles/sender.dir/build: ../lib/libsender.a

.PHONY : src/client/CMakeFiles/sender.dir/build

src/client/CMakeFiles/sender.dir/requires: src/client/CMakeFiles/sender.dir/sender.cpp.o.requires

.PHONY : src/client/CMakeFiles/sender.dir/requires

src/client/CMakeFiles/sender.dir/clean:
	cd /home/tinoryj/code/generaldedupsystem/build/src/client && $(CMAKE_COMMAND) -P CMakeFiles/sender.dir/cmake_clean.cmake
.PHONY : src/client/CMakeFiles/sender.dir/clean

src/client/CMakeFiles/sender.dir/depend:
	cd /home/tinoryj/code/generaldedupsystem/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tinoryj/code/generaldedupsystem /home/tinoryj/code/generaldedupsystem/src/client /home/tinoryj/code/generaldedupsystem/build /home/tinoryj/code/generaldedupsystem/build/src/client /home/tinoryj/code/generaldedupsystem/build/src/client/CMakeFiles/sender.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/client/CMakeFiles/sender.dir/depend

