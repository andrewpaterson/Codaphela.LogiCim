# Linux build of LogiCim (BaseLib + StandardLib + SupportLib + InputLib +
# WindowLib + X11Lib + MicroprocessorLib + LogicimLib + KicadImporterLib + LogiCim's window
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

cmake_minimum_required(VERSION 3.16)
project(LogiCim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

option(BASELIB_ASAN "Build with AddressSanitizer and LeakSanitizer" OFF)

# This solution is in the LogiCim repository (with LogicimLib and KicadImporterLib), which is beside CommonLibraries.
get_filename_component(LOGICIM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
get_filename_component(CODAPHELA_ROOT "${LOGICIM_ROOT}/.." ABSOLUTE)
set(COMMON_DIR "${CODAPHELA_ROOT}/commonlibraries/Libraries")
set(BASELIB_DIR "${COMMON_DIR}/BaseLib")
set(STANDARDLIB_DIR "${COMMON_DIR}/StandardLib")
set(SUPPORTLIB_DIR "${COMMON_DIR}/SupportLib")
set(INPUTLIB_DIR "${COMMON_DIR}/InputLib")
set(WINDOWLIB_DIR "${COMMON_DIR}/WindowLib")
set(X11LIB_DIR "${COMMON_DIR}/X11Lib")
set(MICROPROCESSORLIB_DIR "${COMMON_DIR}/MicroprocessorLib")
set(LOGICIMLIB_DIR "${LOGICIM_ROOT}/Libraries/LogicimLib")
set(KICADLIB_DIR "${COMMON_DIR}/KicadLib")
set(KICADIMPORTERLIB_DIR "${LOGICIM_ROOT}/Libraries/KicadImporterLib")
set(LOGICIM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/LogiCim")


# Reads <ClCompile Include="..."/> entries from a .vcxproj and resolves them
# against the files on disk case-insensitively (Windows paths are not
# case-sensitive).  Files named in EXCLUDE (paths relative to the project,
# any case) are left out.
function(vcxproj_sources OUT_VAR VCXPROJ)
	cmake_parse_arguments(ARG "" "" "EXCLUDE" ${ARGN})
	get_filename_component(PROJ_DIR "${VCXPROJ}" DIRECTORY)

	# Re-run the configure step when Visual Studio adds or removes files.
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${VCXPROJ}")

	file(GLOB_RECURSE DISK_FILES RELATIVE "${PROJ_DIR}" "${PROJ_DIR}/*.cpp" "${PROJ_DIR}/*.c")
	foreach(DISK_FILE ${DISK_FILES})
		string(TOLOWER "${DISK_FILE}" KEY)
		set("DISK_${KEY}" "${DISK_FILE}")
	endforeach()

	set(EXCLUDED "")
	foreach(EXCLUDE_FILE ${ARG_EXCLUDE})
		string(TOLOWER "${EXCLUDE_FILE}" KEY)
		list(APPEND EXCLUDED "${KEY}")
	endforeach()

	file(STRINGS "${VCXPROJ}" LINES REGEX "<ClCompile Include=\"[^\"]+\"")
	set(SOURCES "")
	foreach(LINE ${LINES})
		string(REGEX REPLACE ".*<ClCompile Include=\"([^\"]+)\".*" "\\1" SOURCE "${LINE}")
		string(REPLACE "\\" "/" SOURCE "${SOURCE}")
		string(TOLOWER "${SOURCE}" KEY)
		if(KEY IN_LIST EXCLUDED)
			continue()
		endif()
		if(NOT DEFINED "DISK_${KEY}")
			message(FATAL_ERROR "${VCXPROJ} lists ${SOURCE} but it was not found on disk")
		endif()
		list(APPEND SOURCES "${PROJ_DIR}/${DISK_${KEY}}")
	endforeach()

	set(${OUT_VAR} ${SOURCES} PARENT_SCOPE)
endfunction()


vcxproj_sources(BASELIB_SOURCES "${BASELIB_DIR}/BaseLib.vcxproj")

vcxproj_sources(STANDARDLIB_SOURCES "${STANDARDLIB_DIR}/StandardLib.vcxproj")

vcxproj_sources(SUPPORTLIB_SOURCES "${SUPPORTLIB_DIR}/SupportLib.vcxproj")

vcxproj_sources(INPUTLIB_SOURCES "${INPUTLIB_DIR}/InputLib.vcxproj")

vcxproj_sources(WINDOWLIB_SOURCES "${WINDOWLIB_DIR}/WindowLib.vcxproj")

vcxproj_sources(X11LIB_SOURCES "${X11LIB_DIR}/X11Lib.vcxproj")
vcxproj_sources(MICROPROCESSORLIB_SOURCES "${MICROPROCESSORLIB_DIR}/MicroprocessorLib.vcxproj")
vcxproj_sources(LOGICIMLIB_SOURCES "${LOGICIMLIB_DIR}/LogicimLib.vcxproj")
vcxproj_sources(KICADLIB_SOURCES "${KICADLIB_DIR}/KicadLib.vcxproj")

vcxproj_sources(KICADIMPORTERLIB_SOURCES "${KICADIMPORTERLIB_DIR}/KicadImporterLib.vcxproj")

# LogiCim.cpp (WinMain) is Windows only.
set(LOGICIM_SOURCES
	"${LOGICIM_DIR}/LogiCimLinux.cpp"
	"${LOGICIM_DIR}/CircuitCanvas.cpp"
	"${LOGICIM_DIR}/CircuitCanvasDraw.cpp"
	"${LOGICIM_DIR}/LogicimCircuit.cpp"
	"${LOGICIM_DIR}/LogicimComponentChooser.cpp"
	"${LOGICIM_DIR}/LogicimWindowContent.cpp"
	"${LOGICIM_DIR}/LogicimWindowContentLayout.cpp"
	"${LOGICIM_DIR}/LogicimWindowContentDialogs.cpp"
	"${LOGICIM_DIR}/LogicimWindowContentSubcircuits.cpp"
	"${LOGICIM_DIR}/LogicimWindowData.cpp"
	"${LOGICIM_DIR}/LogicimWindowTick.cpp"
	"${LOGICIM_DIR}/LogicimApp.cpp"
)


# Matches the Visual Studio configurations, except BREAK_ON_ERROR and
# BREAK_ON_WARNING: on Linux BREAK() raises SIGTRAP, which kills the test run
# when no debugger is attached.
add_library(baselib_options INTERFACE)
target_include_directories(baselib_options INTERFACE "${COMMON_DIR}" "${LOGICIM_ROOT}/Libraries")
target_compile_definitions(baselib_options INTERFACE
	LINUX_GNU
	$<$<CONFIG:Debug>:_DEBUG>
	$<$<CONFIG:Debug>:DEBUG>
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)
# -fno-strict-aliasing: MSVC never optimises on the strict aliasing rule and the
# code relies on that (e.g. (CUnknown**)&pcTyped), so optimised builds break without it.
# -Wno-ignored-attributes: g++ drops ALIGN_64 when int64/uint64/float64 is a
# template argument.  Templates that store such a type by value use MSVC_ALIGN
# instead (see PrimitiveTypes.h), so the warning only reports the drop.
# -Wno-write-strings: string literals are passed as char* throughout; MSVC allows it.
# -Wno-multichar: chunk and file IDs are multi-character constants like 'ABCD'.
# -Wno-class-memaccess: the containers memcpy objects with constructors on purpose.
# -Wno-overloaded-virtual: subclasses overload virtuals with other argument types
# (e.g. Save(CObjectWriter*) hides CUnknown::Save(CFileWriter*)) on purpose.
# -Wno-comment: ASCII diagrams in comments end lines with a backslash.
# -Wno-unused-parameter: virtual overrides and callbacks often ignore parameters.
target_compile_options(baselib_options INTERFACE -m64 -fno-strict-aliasing -Wall -Wextra -Wno-ignored-attributes -Wno-write-strings -Wno-multichar -Wno-class-memaccess -Wno-overloaded-virtual -Wno-comment -Wno-unused-parameter)
target_link_options(baselib_options INTERFACE -m64)
if(BASELIB_ASAN)
	target_compile_options(baselib_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
	target_link_options(baselib_options INTERFACE -fsanitize=address)
endif()


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
target_link_libraries(X11Lib PUBLIC WindowLib InputLib SupportLib StandardLib BaseLib X11::X11)

add_library(MicroprocessorLib STATIC ${MICROPROCESSORLIB_SOURCES})
target_link_libraries(MicroprocessorLib PUBLIC StandardLib BaseLib)
add_library(LogicimLib STATIC ${LOGICIMLIB_SOURCES})
target_link_libraries(LogicimLib PUBLIC SupportLib MicroprocessorLib StandardLib BaseLib)
# The KiCad schematic reader (CommonLibraries); KicadImporterLib makes Logicim circuits from what it reads.
add_library(KicadLib STATIC ${KICADLIB_SOURCES})
target_link_libraries(KicadLib PUBLIC BaseLib)

add_library(KicadImporterLib STATIC ${KICADIMPORTERLIB_SOURCES})
target_link_libraries(KicadImporterLib PUBLIC KicadLib LogicimLib SupportLib MicroprocessorLib StandardLib BaseLib)
add_executable(LogiCim ${LOGICIM_SOURCES})
target_include_directories(LogiCim PRIVATE "${LOGICIM_DIR}")
target_link_libraries(LogiCim PRIVATE X11Lib WindowLib InputLib KicadImporterLib KicadLib LogicimLib MicroprocessorLib SupportLib StandardLib BaseLib)
