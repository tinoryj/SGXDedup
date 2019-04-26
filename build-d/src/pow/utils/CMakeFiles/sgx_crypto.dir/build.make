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
include src/pow/utils/CMakeFiles/sgx_crypto.dir/depend.make

# Include the progress variables for this target.
include src/pow/utils/CMakeFiles/sgx_crypto.dir/progress.make

# Include the compile flags for this target's objects.
include src/pow/utils/CMakeFiles/sgx_crypto.dir/flags.make

src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o: src/pow/utils/CMakeFiles/sgx_crypto.dir/flags.make
src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o: ../src/pow/utils/crypto.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o"
	cd /home/tinoryj/code/generaldedupsystem/build/src/pow/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/sgx_crypto.dir/crypto.c.o   -c /home/tinoryj/code/generaldedupsystem/src/pow/utils/crypto.c

src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/sgx_crypto.dir/crypto.c.i"
	cd /home/tinoryj/code/generaldedupsystem/build/src/pow/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/tinoryj/code/generaldedupsystem/src/pow/utils/crypto.c > CMakeFiles/sgx_crypto.dir/crypto.c.i

src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/sgx_crypto.dir/crypto.c.s"
	cd /home/tinoryj/code/generaldedupsystem/build/src/pow/utils && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/tinoryj/code/generaldedupsystem/src/pow/utils/crypto.c -o CMakeFiles/sgx_crypto.dir/crypto.c.s

src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.requires:

.PHONY : src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.requires

src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.provides: src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.requires
	$(MAKE) -f src/pow/utils/CMakeFiles/sgx_crypto.dir/build.make src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.provides.build
.PHONY : src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.provides

src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.provides.build: src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o


# Object files for target sgx_crypto
sgx_crypto_OBJECTS = \
"CMakeFiles/sgx_crypto.dir/crypto.c.o"

# External object files for target sgx_crypto
sgx_crypto_EXTERNAL_OBJECTS =

../lib/libsgx_crypto.a: src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o
../lib/libsgx_crypto.a: src/pow/utils/CMakeFiles/sgx_crypto.dir/build.make
../lib/libsgx_crypto.a: src/pow/utils/CMakeFiles/sgx_crypto.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/tinoryj/code/generaldedupsystem/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library ../../../../lib/libsgx_crypto.a"
	cd /home/tinoryj/code/generaldedupsystem/build/src/pow/utils && $(CMAKE_COMMAND) -P CMakeFiles/sgx_crypto.dir/cmake_clean_target.cmake
	cd /home/tinoryj/code/generaldedupsystem/build/src/pow/utils && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/sgx_crypto.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/pow/utils/CMakeFiles/sgx_crypto.dir/build: ../lib/libsgx_crypto.a

.PHONY : src/pow/utils/CMakeFiles/sgx_crypto.dir/build

src/pow/utils/CMakeFiles/sgx_crypto.dir/requires: src/pow/utils/CMakeFiles/sgx_crypto.dir/crypto.c.o.requires

.PHONY : src/pow/utils/CMakeFiles/sgx_crypto.dir/requires

src/pow/utils/CMakeFiles/sgx_crypto.dir/clean:
	cd /home/tinoryj/code/generaldedupsystem/build/src/pow/utils && $(CMAKE_COMMAND) -P CMakeFiles/sgx_crypto.dir/cmake_clean.cmake
.PHONY : src/pow/utils/CMakeFiles/sgx_crypto.dir/clean

src/pow/utils/CMakeFiles/sgx_crypto.dir/depend:
	cd /home/tinoryj/code/generaldedupsystem/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tinoryj/code/generaldedupsystem /home/tinoryj/code/generaldedupsystem/src/pow/utils /home/tinoryj/code/generaldedupsystem/build /home/tinoryj/code/generaldedupsystem/build/src/pow/utils /home/tinoryj/code/generaldedupsystem/build/src/pow/utils/CMakeFiles/sgx_crypto.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/pow/utils/CMakeFiles/sgx_crypto.dir/depend

