# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.21

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
CMAKE_SOURCE_DIR = /root/flexran-rtc

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /root/flexran-rtc/build

# Include any dependencies generated for this target.
include src/core/CMakeFiles/RTC_CORE_LIB.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/core/CMakeFiles/RTC_CORE_LIB.dir/compiler_depend.make

# Include the progress variables for this target.
include src/core/CMakeFiles/RTC_CORE_LIB.dir/progress.make

# Include the compile flags for this target's objects.
include src/core/CMakeFiles/RTC_CORE_LIB.dir/flags.make

src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/flags.make
src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o: ../src/core/rt_wrapper.cc
src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/flexran-rtc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o -MF CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o.d -o CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o -c /root/flexran-rtc/src/core/rt_wrapper.cc

src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.i"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/flexran-rtc/src/core/rt_wrapper.cc > CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.i

src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.s"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/flexran-rtc/src/core/rt_wrapper.cc -o CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.s

src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/flags.make
src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o: ../src/core/task_manager.cc
src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/flexran-rtc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o -MF CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o.d -o CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o -c /root/flexran-rtc/src/core/task_manager.cc

src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.i"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/flexran-rtc/src/core/task_manager.cc > CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.i

src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.s"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/flexran-rtc/src/core/task_manager.cc -o CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.s

src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/flags.make
src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o: ../src/core/rt_task.cc
src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/flexran-rtc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o -MF CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o.d -o CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o -c /root/flexran-rtc/src/core/rt_task.cc

src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.i"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/flexran-rtc/src/core/rt_task.cc > CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.i

src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.s"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/flexran-rtc/src/core/rt_task.cc -o CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.s

src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/flags.make
src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o: ../src/core/requests_manager.cc
src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o: src/core/CMakeFiles/RTC_CORE_LIB.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/root/flexran-rtc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o -MF CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o.d -o CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o -c /root/flexran-rtc/src/core/requests_manager.cc

src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.i"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /root/flexran-rtc/src/core/requests_manager.cc > CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.i

src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.s"
	cd /root/flexran-rtc/build/src/core && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /root/flexran-rtc/src/core/requests_manager.cc -o CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.s

# Object files for target RTC_CORE_LIB
RTC_CORE_LIB_OBJECTS = \
"CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o" \
"CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o" \
"CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o" \
"CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o"

# External object files for target RTC_CORE_LIB
RTC_CORE_LIB_EXTERNAL_OBJECTS =

src/core/libRTC_CORE_LIB.a: src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_wrapper.cc.o
src/core/libRTC_CORE_LIB.a: src/core/CMakeFiles/RTC_CORE_LIB.dir/task_manager.cc.o
src/core/libRTC_CORE_LIB.a: src/core/CMakeFiles/RTC_CORE_LIB.dir/rt_task.cc.o
src/core/libRTC_CORE_LIB.a: src/core/CMakeFiles/RTC_CORE_LIB.dir/requests_manager.cc.o
src/core/libRTC_CORE_LIB.a: src/core/CMakeFiles/RTC_CORE_LIB.dir/build.make
src/core/libRTC_CORE_LIB.a: src/core/CMakeFiles/RTC_CORE_LIB.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/root/flexran-rtc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking CXX static library libRTC_CORE_LIB.a"
	cd /root/flexran-rtc/build/src/core && $(CMAKE_COMMAND) -P CMakeFiles/RTC_CORE_LIB.dir/cmake_clean_target.cmake
	cd /root/flexran-rtc/build/src/core && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/RTC_CORE_LIB.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/core/CMakeFiles/RTC_CORE_LIB.dir/build: src/core/libRTC_CORE_LIB.a
.PHONY : src/core/CMakeFiles/RTC_CORE_LIB.dir/build

src/core/CMakeFiles/RTC_CORE_LIB.dir/clean:
	cd /root/flexran-rtc/build/src/core && $(CMAKE_COMMAND) -P CMakeFiles/RTC_CORE_LIB.dir/cmake_clean.cmake
.PHONY : src/core/CMakeFiles/RTC_CORE_LIB.dir/clean

src/core/CMakeFiles/RTC_CORE_LIB.dir/depend:
	cd /root/flexran-rtc/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /root/flexran-rtc /root/flexran-rtc/src/core /root/flexran-rtc/build /root/flexran-rtc/build/src/core /root/flexran-rtc/build/src/core/CMakeFiles/RTC_CORE_LIB.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/core/CMakeFiles/RTC_CORE_LIB.dir/depend

