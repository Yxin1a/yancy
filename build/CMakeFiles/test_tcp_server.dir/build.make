# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/yangxin/sylar-master

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/yangxin/sylar-master/build

# Include any dependencies generated for this target.
include CMakeFiles/test_tcp_server.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/test_tcp_server.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/test_tcp_server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/test_tcp_server.dir/flags.make

CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o: CMakeFiles/test_tcp_server.dir/flags.make
CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o: /home/yangxin/sylar-master/tests/test_tcp_server.cpp
CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o: CMakeFiles/test_tcp_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/yangxin/sylar-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_tcp_server.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o -MF CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o.d -o CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o -c /home/yangxin/sylar-master/tests/test_tcp_server.cpp

CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_tcp_server.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/yangxin/sylar-master/tests/test_tcp_server.cpp > CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.i

CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) -D__FILE__=\"tests/test_tcp_server.cpp\" $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/yangxin/sylar-master/tests/test_tcp_server.cpp -o CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.s

# Object files for target test_tcp_server
test_tcp_server_OBJECTS = \
"CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o"

# External object files for target test_tcp_server
test_tcp_server_EXTERNAL_OBJECTS =

/home/yangxin/sylar-master/bin/test_tcp_server: CMakeFiles/test_tcp_server.dir/tests/test_tcp_server.cpp.o
/home/yangxin/sylar-master/bin/test_tcp_server: CMakeFiles/test_tcp_server.dir/build.make
/home/yangxin/sylar-master/bin/test_tcp_server: /home/yangxin/sylar-master/lib/libsylar_server.so
/home/yangxin/sylar-master/bin/test_tcp_server: /usr/local/lib/libz.so
/home/yangxin/sylar-master/bin/test_tcp_server: /usr/lib/x86_64-linux-gnu/libssl.so
/home/yangxin/sylar-master/bin/test_tcp_server: /usr/lib/x86_64-linux-gnu/libcrypto.so
/home/yangxin/sylar-master/bin/test_tcp_server: CMakeFiles/test_tcp_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/yangxin/sylar-master/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable /home/yangxin/sylar-master/bin/test_tcp_server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_tcp_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/test_tcp_server.dir/build: /home/yangxin/sylar-master/bin/test_tcp_server
.PHONY : CMakeFiles/test_tcp_server.dir/build

CMakeFiles/test_tcp_server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/test_tcp_server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/test_tcp_server.dir/clean

CMakeFiles/test_tcp_server.dir/depend:
	cd /home/yangxin/sylar-master/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/yangxin/sylar-master /home/yangxin/sylar-master /home/yangxin/sylar-master/build /home/yangxin/sylar-master/build /home/yangxin/sylar-master/build/CMakeFiles/test_tcp_server.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/test_tcp_server.dir/depend

