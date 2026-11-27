# Linux build of TestLogicimLib (BaseLib + TestLib + StandardLib + SupportLib +
# MicroprocessorLib + LogicimLib + TestLogicimLib).
#
# Builds 32-bit (-m32, needs g++-multilib) by default.  Add -DBASELIB_64=ON
# for a 64-bit build, in its own build directory:
#
#   cmake -S LogiCim/Tests/TestLogicimLib -B build/TestLogicimLib64 -G Ninja -DBASELIB_64=ON
#
#   cmake -S LogiCim/Tests/TestLogicimLib -B build/TestLogicimLib -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build/TestLogicimLib
#   ctest --test-dir build/TestLogicimLib -V
#
# Add -DBASELIB_ASAN=ON for AddressSanitizer and LeakSanitizer.
#
# Source lists are read from the .vcxproj files so Visual Studio stays the
# source of truth.  Building re-runs the configure step automatically when a
# .vcxproj changes.  Test directories can contain .cpp files that are test data,
# so never glob *.cpp to find sources.

cmake_minimum_required(VERSION 3.19)
project(TestLogicimLib CXX)


# This test is in the LogiCim repository (with LogicimLib and KicadImporterLib), which is beside CommonLibraries.
get_filename_component(LOGICIM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(CODAPHELA_ROOT "${LOGICIM_ROOT}/.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/CommonLibraries/Libraries")
set(BASELIB_DIR "${COMMON_DIR}/BaseLib")
set(TESTLIB_DIR "${COMMON_DIR}/TestLib")
set(STANDARDLIB_DIR "${COMMON_DIR}/StandardLib")
set(SUPPORTLIB_DIR "${COMMON_DIR}/SupportLib")
set(MICROPROCESSORLIB_DIR "${COMMON_DIR}/MicroprocessorLib")
set(LOGICIMLIB_DIR "${LOGICIM_ROOT}/Libraries/LogicimLib")
set(TESTLOGICIMLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/TestLogicimLib")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")


vcxproj_sources(BASELIB_SOURCES "${BASELIB_DIR}/BaseLib.vcxproj")

vcxproj_sources(TESTLIB_SOURCES "${TESTLIB_DIR}/TestLib.vcxproj")

vcxproj_sources(STANDARDLIB_SOURCES "${STANDARDLIB_DIR}/StandardLib.vcxproj")

vcxproj_sources(SUPPORTLIB_SOURCES "${SUPPORTLIB_DIR}/SupportLib.vcxproj")

vcxproj_sources(MICROPROCESSORLIB_SOURCES "${MICROPROCESSORLIB_DIR}/MicroprocessorLib.vcxproj")

vcxproj_sources(LOGICIMLIB_SOURCES "${LOGICIMLIB_DIR}/LogicimLib.vcxproj")

vcxproj_sources(TESTLOGICIMLIB_SOURCES "${TESTLOGICIMLIB_DIR}/TestLogicimLib.vcxproj")


codaphela_options("${COMMON_DIR}" "${LOGICIM_ROOT}/Libraries")


# CMutex uses pthreads on Linux.
find_package(Threads REQUIRED)

add_library(BaseLib STATIC ${BASELIB_SOURCES})
target_link_libraries(BaseLib PUBLIC baselib_options)
target_link_libraries(BaseLib PUBLIC Threads::Threads)

# Uses AVX intrinsics unconditionally; MSVC allows that without a flag, g++ does not.

add_library(TestLib STATIC ${TESTLIB_SOURCES})
target_link_libraries(TestLib PUBLIC BaseLib)

add_library(StandardLib STATIC ${STANDARDLIB_SOURCES})
target_link_libraries(StandardLib PUBLIC BaseLib)

# The Debug configurations of StandardLib (only) define DEBUG_POINTER.
target_compile_definitions(StandardLib PRIVATE $<$<CONFIG:Debug>:DEBUG_POINTER>)

add_library(SupportLib STATIC ${SUPPORTLIB_SOURCES})
target_link_libraries(SupportLib PUBLIC StandardLib BaseLib)

# MicroprocessorLib depends on StandardLib only, never SupportLib.
add_library(MicroprocessorLib STATIC ${MICROPROCESSORLIB_SOURCES})
target_link_libraries(MicroprocessorLib PUBLIC StandardLib BaseLib)

add_library(LogicimLib STATIC ${LOGICIMLIB_SOURCES})
target_link_libraries(LogicimLib PUBLIC SupportLib MicroprocessorLib StandardLib BaseLib)

add_executable(TestLogicimLib ${TESTLOGICIMLIB_SOURCES})
target_link_libraries(TestLogicimLib PRIVATE TestLib LogicimLib MicroprocessorLib SupportLib StandardLib BaseLib)

# The tests read their input (Input/ ...) relative to the project directory,
# as Visual Studio does.
enable_testing()
add_test(NAME TestLogicimLib COMMAND TestLogicimLib WORKING_DIRECTORY "${TESTLOGICIMLIB_DIR}")
