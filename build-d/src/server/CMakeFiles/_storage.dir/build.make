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
include src/server/CMakeFiles/_storage.dir/depend.make

# Include the progress variables for this target.
include src/server/CMakeFiles/_storage.dir/progress.make

# Include the compile flags for this target's objects.
include src/server/CMakeFiles/_storage.dir/flags.make

src/server/CMakeFiles/_storage.dir/_storage.cpp.o: src/server/CMakeFiles/_storage.dir/flags.make
src/server/CMakeFiles/_storage.dir/_storage.cpp.o: ../src/server/_storage.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/server/CMakeFiles/_storage.dir/_storage.cpp.o"
	cd /home/tinoryj/code/generaldedupsystem/build/src/server && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/_storage.dir/_storage.cpp.o -c /home/tinoryj/code/generaldedupsystem/src/server/_storage.cpp

src/server/CMakeFiles/_storage.dir/_storage.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/_storage.dir/_storage.cpp.i"
	cd /home/tinoryj/code/generaldedupsystem/build/src/server && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tinoryj/code/generaldedupsystem/src/server/_storage.cpp > CMakeFiles/_storage.dir/_storage.cpp.i

src/server/CMakeFiles/_storage.dir/_storage.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/_storage.dir/_storage.cpp.s"
	cd /home/tinoryj/code/generaldedupsystem/build/src/server && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tinoryj/code/generaldedupsystem/src/server/_storage.cpp -o CMakeFiles/_storage.dir/_storage.cpp.s

src/server/CMakeFiles/_storage.dir/_storage.cpp.o.requires:

.PHONY : src/server/CMakeFiles/_storage.dir/_storage.cpp.o.requires

src/server/CMakeFiles/_storage.dir/_storage.cpp.o.provides: src/server/CMakeFiles/_storage.dir/_storage.cpp.o.requires
	$(MAKE) -f src/server/CMakeFiles/_storage.dir/build.make src/server/CMakeFiles/_storage.dir/_storage.cpp.o.provides.build
.PHONY : src/server/CMakeFiles/_storage.dir/_storage.cpp.o.provides

src/server/CMakeFiles/_storage.dir/_storage.cpp.o.provides.build: src/server/CMakeFiles/_storage.dir/_storage.cpp.o


# Object files for target _storage
_storage_OBJECTS = \
"CMakeFiles/_storage.dir/_storage.cpp.o"

# External object files for target _storage
_storage_EXTERNAL_OBJECTS =

../lib/lib_storage.a: src/server/CMakeFiles/_storage.dir/_storage.cpp.o
../lib/lib_storage.a: src/server/CMakeFiles/_storage.dir/build.make
../lib/lib_storage.a: src/server/CMakeFiles/_storage.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../../lib/lib_storage.a"
	cd /home/tinoryj/code/generaldedupsystem/build/src/server && $(CMAKE_COMMAND) -P CMakeFiles/_storage.dir/cmake_clean_target.cmake
	cd /home/tinoryj/code/generaldedupsystem/build/src/server && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/_storage.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/server/CMakeFiles/_storage.dir/build: ../lib/lib_storage.a

.PHONY : src/server/CMakeFiles/_storage.dir/build

src/server/CMakeFiles/_storage.dir/requires: src/server/CMakeFiles/_storage.dir/_storage.cpp.o.requires

.PHONY : src/server/CMakeFiles/_storage.dir/requires

src/server/CMakeFiles/_storage.dir/clean:
	cd /home/tinoryj/code/generaldedupsystem/build/src/server && $(CMAKE_COMMAND) -P CMakeFiles/_storage.dir/cmake_clean.cmake
.PHONY : src/server/CMakeFiles/_storage.dir/clean

src/server/CMakeFiles/_storage.dir/depend:
	cd /home/tinoryj/code/generaldedupsystem/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tinoryj/code/generaldedupsystem /home/tinoryj/code/generaldedupsystem/src/server /home/tinoryj/code/generaldedupsystem/build /home/tinoryj/code/generaldedupsystem/build/src/server /home/tinoryj/code/generaldedupsystem/build/src/server/CMakeFiles/_storage.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/server/CMakeFiles/_storage.dir/depend

