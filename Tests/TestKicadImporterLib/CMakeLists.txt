# Linux build of TestKicadImporterLib (BaseLib + TestLib + StandardLib + SupportLib +
# MicroprocessorLib + LogicimLib + KicadImporterLib + TestKicadImporterLib).
#
# Builds 32-bit (-m32, needs g++-multilib) by default.  Add -DBASELIB_64=ON
# for a 64-bit build, in its own build directory:
#
#   cmake -S LogiCim/Tests/TestKicadImporterLib -B build/TestKicadImporterLib64 -G Ninja -DBASELIB_64=ON
#
#   cmake -S LogiCim/Tests/TestKicadImporterLib -B build/TestKicadImporterLib -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build/TestKicadImporterLib
#   ctest --test-dir build/TestKicadImporterLib -V
#
# Add -DBASELIB_ASAN=ON for AddressSanitizer and LeakSanitizer.
#
# Source lists are read from the .vcxproj files so Visual Studio stays the
# source of truth.  Building re-runs the configure step automatically when a
# .vcxproj changes.  Test directories can contain .cpp files that are test data,
# so never glob *.cpp to find sources.

cmake_minimum_required(VERSION 3.19)
project(TestKicadImporterLib CXX)


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
set(KICADLIB_DIR "${COMMON_DIR}/KicadLib")
set(KICADIMPORTERLIB_DIR "${LOGICIM_ROOT}/Libraries/KicadImporterLib")
set(TESTKICADIMPORTERLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/TestKicadImporterLib")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")


vcxproj_sources(BASELIB_SOURCES "${BASELIB_DIR}/BaseLib.vcxproj")

vcxproj_sources(TESTLIB_SOURCES "${TESTLIB_DIR}/TestLib.vcxproj")

vcxproj_sources(STANDARDLIB_SOURCES "${STANDARDLIB_DIR}/StandardLib.vcxproj")

vcxproj_sources(SUPPORTLIB_SOURCES "${SUPPORTLIB_DIR}/SupportLib.vcxproj")

vcxproj_sources(MICROPROCESSORLIB_SOURCES "${MICROPROCESSORLIB_DIR}/MicroprocessorLib.vcxproj")

vcxproj_sources(LOGICIMLIB_SOURCES "${LOGICIMLIB_DIR}/LogicimLib.vcxproj")

vcxproj_sources(KICADLIB_SOURCES "${KICADLIB_DIR}/KicadLib.vcxproj")

vcxproj_sources(KICADIMPORTERLIB_SOURCES "${KICADIMPORTERLIB_DIR}/KicadImporterLib.vcxproj")

vcxproj_sources(TESTKICADIMPORTERLIB_SOURCES "${TESTKICADIMPORTERLIB_DIR}/TestKicadImporterLib.vcxproj")


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

# The KiCad schematic reader (CommonLibraries); KicadImporterLib makes Logicim circuits from what it reads.
add_library(KicadLib STATIC ${KICADLIB_SOURCES})
target_link_libraries(KicadLib PUBLIC BaseLib)

# KicadImporterLib uses LogicimLib to write Logicim circuits.  LogicimLib must never depend on it.
add_library(KicadImporterLib STATIC ${KICADIMPORTERLIB_SOURCES})
target_link_libraries(KicadImporterLib PUBLIC KicadLib LogicimLib MicroprocessorLib SupportLib StandardLib BaseLib)

add_executable(TestKicadImporterLib ${TESTKICADIMPORTERLIB_SOURCES})
target_link_libraries(TestKicadImporterLib PRIVATE TestLib KicadImporterLib KicadLib LogicimLib MicroprocessorLib SupportLib StandardLib BaseLib)

# The tests read their input (Input/ ...) relative to the project directory,
# as Visual Studio does.
enable_testing()
add_test(NAME TestKicadImporterLib COMMAND TestKicadImporterLib WORKING_DIRECTORY "${TESTKICADIMPORTERLIB_DIR}")
