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
include ExampleCode/CMakeFiles/Boost_seriazation.dir/depend.make

# Include the progress variables for this target.
include ExampleCode/CMakeFiles/Boost_seriazation.dir/progress.make

# Include the compile flags for this target's objects.
include ExampleCode/CMakeFiles/Boost_seriazation.dir/flags.make

ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o: ExampleCode/CMakeFiles/Boost_seriazation.dir/flags.make
ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o: ../ExampleCode/Boost_seriazation.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o -c /home/tinoryj/code/generaldedupsystem/ExampleCode/Boost_seriazation.cpp

ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.i"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/tinoryj/code/generaldedupsystem/ExampleCode/Boost_seriazation.cpp > CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.i

ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.s"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/tinoryj/code/generaldedupsystem/ExampleCode/Boost_seriazation.cpp -o CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.s

ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.requires:

.PHONY : ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.requires

ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.provides: ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.requires
	$(MAKE) -f ExampleCode/CMakeFiles/Boost_seriazation.dir/build.make ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.provides.build
.PHONY : ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.provides

ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.provides.build: ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o


# Object files for target Boost_seriazation
Boost_seriazation_OBJECTS = \
"CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o"

# External object files for target Boost_seriazation
Boost_seriazation_EXTERNAL_OBJECTS =

../bin/Boost_seriazation: ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o
../bin/Boost_seriazation: ExampleCode/CMakeFiles/Boost_seriazation.dir/build.make
../bin/Boost_seriazation: ExampleCode/CMakeFiles/Boost_seriazation.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/Boost_seriazation"
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Boost_seriazation.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
ExampleCode/CMakeFiles/Boost_seriazation.dir/build: ../bin/Boost_seriazation

.PHONY : ExampleCode/CMakeFiles/Boost_seriazation.dir/build

ExampleCode/CMakeFiles/Boost_seriazation.dir/requires: ExampleCode/CMakeFiles/Boost_seriazation.dir/Boost_seriazation.cpp.o.requires

.PHONY : ExampleCode/CMakeFiles/Boost_seriazation.dir/requires

ExampleCode/CMakeFiles/Boost_seriazation.dir/clean:
	cd /home/tinoryj/code/generaldedupsystem/build/ExampleCode && $(CMAKE_COMMAND) -P CMakeFiles/Boost_seriazation.dir/cmake_clean.cmake
.PHONY : ExampleCode/CMakeFiles/Boost_seriazation.dir/clean

ExampleCode/CMakeFiles/Boost_seriazation.dir/depend:
	cd /home/tinoryj/code/generaldedupsystem/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tinoryj/code/generaldedupsystem /home/tinoryj/code/generaldedupsystem/ExampleCode /home/tinoryj/code/generaldedupsystem/build /home/tinoryj/code/generaldedupsystem/build/ExampleCode /home/tinoryj/code/generaldedupsystem/build/ExampleCode/CMakeFiles/Boost_seriazation.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : ExampleCode/CMakeFiles/Boost_seriazation.dir/depend

