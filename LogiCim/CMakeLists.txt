# Linux build of LogiCim (BaseLib + StandardLib + SupportLib + InputLib +
# WindowLib + X11Lib + MicroprocessorLib + ThreadLib + LogicimLib + KicadImporterLib + LogiCim's window
# sources).
#
# 64-bit only: there is no 32-bit libX11 installed.  Needs libx11-dev.
#
# From the folder the LogiCim repository and CommonLibraries are in:
#
#   cmake -S LogiCim/LogiCim -B build/LogiCim -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build/LogiCim
#   cd LogiCim/LogiCim/LogiCim && ../../../build/LogiCim/LogiCim
#
# Add -DBASELIB_ASAN=ON for AddressSanitizer and LeakSanitizer.
#
# Library source lists are read from the .vcxproj files so Visual Studio stays
# the source of truth.

cmake_minimum_required(VERSION 3.19)
project(LogiCim CXX)


# This solution is in the LogiCim repository (with LogicimLib and KicadImporterLib), which is beside CommonLibraries.
get_filename_component(LOGICIM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(CODAPHELA_ROOT "${LOGICIM_ROOT}/.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/CommonLibraries/Libraries")
set(BASELIB_DIR "${COMMON_DIR}/BaseLib")
set(STANDARDLIB_DIR "${COMMON_DIR}/StandardLib")
set(SUPPORTLIB_DIR "${COMMON_DIR}/SupportLib")
set(INPUTLIB_DIR "${COMMON_DIR}/InputLib")
set(WINDOWLIB_DIR "${COMMON_DIR}/WindowLib")
set(X11LIB_DIR "${COMMON_DIR}/X11Lib")
set(MICROPROCESSORLIB_DIR "${COMMON_DIR}/MicroprocessorLib")
set(THREADLIB_DIR "${COMMON_DIR}/ThreadLib")
set(LOGICIMLIB_DIR "${LOGICIM_ROOT}/Libraries/LogicimLib")
set(KICADLIB_DIR "${COMMON_DIR}/KicadLib")
set(KICADIMPORTERLIB_DIR "${LOGICIM_ROOT}/Libraries/KicadImporterLib")
set(LOGICIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/LogiCim")

include("${COMMON_DIR}/../CMake/Codaphela.cmake")


vcxproj_sources(BASELIB_SOURCES "${BASELIB_DIR}/BaseLib.vcxproj")

vcxproj_sources(STANDARDLIB_SOURCES "${STANDARDLIB_DIR}/StandardLib.vcxproj")

vcxproj_sources(SUPPORTLIB_SOURCES "${SUPPORTLIB_DIR}/SupportLib.vcxproj")

vcxproj_sources(INPUTLIB_SOURCES "${INPUTLIB_DIR}/InputLib.vcxproj")

vcxproj_sources(WINDOWLIB_SOURCES "${WINDOWLIB_DIR}/WindowLib.vcxproj")

vcxproj_sources(X11LIB_SOURCES "${X11LIB_DIR}/X11Lib.vcxproj")
vcxproj_sources(MICROPROCESSORLIB_SOURCES "${MICROPROCESSORLIB_DIR}/MicroprocessorLib.vcxproj")
vcxproj_sources(THREADLIB_SOURCES "${THREADLIB_DIR}/ThreadLib.vcxproj")
vcxproj_sources(LOGICIMLIB_SOURCES "${LOGICIMLIB_DIR}/LogicimLib.vcxproj")
vcxproj_sources(KICADLIB_SOURCES "${KICADLIB_DIR}/KicadLib.vcxproj")

vcxproj_sources(KICADIMPORTERLIB_SOURCES "${KICADIMPORTERLIB_DIR}/KicadImporterLib.vcxproj")

vcxproj_sources(LOGICIM_SOURCES "${LOGICIM_DIR}/LogiCim.vcxproj")


codaphela_options(ONLY_64 "${COMMON_DIR}" "${LOGICIM_ROOT}/Libraries")


# CMutex uses pthreads on Linux.
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_library(BaseLib STATIC ${BASELIB_SOURCES})
target_link_libraries(BaseLib PUBLIC baselib_options)
target_link_libraries(BaseLib PUBLIC Threads::Threads)

# Uses AVX intrinsics unconditionally; MSVC allows that without a flag, g++ does not.

add_library(StandardLib STATIC ${STANDARDLIB_SOURCES})
target_link_libraries(StandardLib PUBLIC BaseLib)

add_library(SupportLib STATIC ${SUPPORTLIB_SOURCES})
target_link_libraries(SupportLib PUBLIC StandardLib BaseLib)

add_library(InputLib STATIC ${INPUTLIB_SOURCES})
target_link_libraries(InputLib PUBLIC StandardLib BaseLib)

add_library(WindowLib STATIC ${WINDOWLIB_SOURCES})
target_link_libraries(WindowLib PUBLIC InputLib SupportLib StandardLib BaseLib)

add_library(X11Lib STATIC ${X11LIB_SOURCES})
target_link_libraries(X11Lib PUBLIC WindowLib InputLib SupportLib StandardLib BaseLib X11::X11 X11::Xext)

add_library(MicroprocessorLib STATIC ${MICROPROCESSORLIB_SOURCES})
target_link_libraries(MicroprocessorLib PUBLIC StandardLib BaseLib)

add_library(ThreadLib STATIC ${THREADLIB_SOURCES})
target_link_libraries(ThreadLib PUBLIC BaseLib)
add_library(LogicimLib STATIC ${LOGICIMLIB_SOURCES})
target_link_libraries(LogicimLib PUBLIC SupportLib MicroprocessorLib StandardLib BaseLib)
# The KiCad schematic reader (CommonLibraries); KicadImporterLib makes Logicim circuits from what it reads.
add_library(KicadLib STATIC ${KICADLIB_SOURCES})
target_link_libraries(KicadLib PUBLIC BaseLib)

add_library(KicadImporterLib STATIC ${KICADIMPORTERLIB_SOURCES})
target_link_libraries(KicadImporterLib PUBLIC KicadLib LogicimLib SupportLib MicroprocessorLib StandardLib BaseLib)
add_executable(LogiCim ${LOGICIM_SOURCES})
target_include_directories(LogiCim PRIVATE "${LOGICIM_DIR}")
target_link_libraries(LogiCim PRIVATE X11Lib WindowLib InputLib KicadImporterLib KicadLib LogicimLib MicroprocessorLib ThreadLib SupportLib StandardLib BaseLib)
