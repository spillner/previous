
cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(Previous)

SET(APP_NAME "Previous")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckCCompilerFlag)
include(CheckStructHasMember)
include(DistClean)

# Set build type to "Release" if user did not specify any build type yet
# Other possible values: Debug, Release, RelWithDebInfo and MinSizeRel
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# set(CMAKE_VERBOSE_MAKEFILE 1)

# ##########################
# Conditional build features
# ##########################

set(ENABLE_TRACING 1
    CACHE BOOL "Enable tracing messages for debugging")

if(APPLE)
	set(ENABLE_OSX_BUNDLE 1
	    CACHE BOOL "Built Previous as Mac OS X application bundle")
	# set(CMAKE_OSX_ARCHITECTURES "i386" CACHE STRING "Target architectures" FORCE)
	# set(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.6.sdk" CACHE STRING "10.6 SDK" FORCE)
	# set(CMAKE_OSX_DEPLOYMENT_TARGET "10.5" CACHE STRING "Target Min 10.5" FORCE)
	set(ADDITIONAL_INCLUDES ${FRAMEWORKS})
	set_source_files_properties(${FRAMEWORKS} PROPERTIES MACOSX_PACKAGE_LOCATION Frameworks)
else()
	set(ENABLE_OSX_BUNDLE 0
	    CACHE BOOL "Built Hatari as Mac OS X application bundle")
endif(APPLE)

# ####################
# Check for libraries:
# ####################

find_package(SDL2)
if(NOT SDL2_FOUND)
	message(FATAL_ERROR "SDL2 library not found!")
endif(NOT SDL2_FOUND)

find_package(Math)

find_package(Readline)
if(Readline_FOUND)
	set(HAVE_LIBREADLINE 1)
endif(Readline_FOUND)

find_package(ZLIB)
if(ZLIB_FOUND)
	set(HAVE_LIBZ 1)
endif(ZLIB_FOUND)

find_package(PNG)
if(PNG_FOUND)
	set(HAVE_LIBPNG 1)
endif(PNG_FOUND)

find_package(PCAP)
if(PCAP_FOUND)
	set(HAVE_PCAP 1)
endif(PCAP_FOUND)

# ################
# CPP Definitions:
# ################

add_definitions(-DCONFDIR=\"/etc\")

# Test for large file support:
execute_process(COMMAND getconf LFS_CFLAGS
                OUTPUT_VARIABLE DETECTED_LFS_CFLAGS
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(DETECTED_LFS_CFLAGS)
	add_definitions(${DETECTED_LFS_CFLAGS})
	# message(STATUS "Large filesystem flags: ${DETECTED_LFS_CFLAGS}")
endif(DETECTED_LFS_CFLAGS)

# Additional CFLAGS suggested by the SDL library:
execute_process(COMMAND pkg-config --cflags-only-other sdl2
		OUTPUT_VARIABLE DETECTED_SDL_CFLAGS
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(DETECTED_SDL_CFLAGS)
	add_definitions(${DETECTED_SDL_CFLAGS})
	# message(STATUS "Additional CFLAGS of SDL: ${DETECTED_SDL_CFLAGS}")
endif(DETECTED_SDL_CFLAGS)

# ###########################
# Check for optional headers:
# ###########################

check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(limits.h HAVE_LIMITS_H)
check_include_files(sys/syslimits.h HAVE_SYS_SYSLIMITS_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(tchar.h HAVE_TCHAR_H)
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)

# #############################
# Check for optional functions:
# #############################

check_function_exists(setenv HAVE_SETENV)
check_function_exists(select HAVE_SELECT)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(nanosleep HAVE_NANOSLEEP)
check_function_exists(alphasort HAVE_ALPHASORT)
check_function_exists(scandir HAVE_SCANDIR)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(posix_memalign HAVE_POSIX_MEMALIGN)
check_function_exists(aligned_alloc HAVE_ALIGNED_ALLOC)
check_function_exists(_aligned_alloc HAVE__ALIGNED_ALLOC)

# ########################################
# Check for non-POSIXness on MacOS et al.:
# ########################################
message("-- The following HAVE_STRUCT_* tests are checking for non-POSIX systems such as MacOS; 'Failed' does not indicate a problem")
CHECK_STRUCT_HAS_MEMBER("struct stat" st_atimespec.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_ATIMESPEC)
CHECK_STRUCT_HAS_MEMBER("struct stat" st_mtimespec.tv_nsec
    "sys/types.h;sys/stat.h" HAVE_STRUCT_STAT_ST_MTIMESPEC)
# Linux glibc provides a _D_EXACT_NAMELEN macro to work around this exact issue, but other supported platforms may not
CHECK_STRUCT_HAS_MEMBER("struct dirent" d_namelen
	"dirent.h" HAVE_STRUCT_DIRENT_D_NAMELEN)


# #############
# Other CFLAGS:
# #############

# Warning flags:
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "-Wcast-qual -g -O0 -Wbad-function-cast -Wpointer-arith ${CMAKE_C_FLAGS} -D__USE_MINGW_ANSI_STDIO=1")
	set(CMAKE_C_FLAGS "-Wmissing-prototypes -Wstrict-prototypes ${CMAKE_C_FLAGS}")
	set(CMAKE_C_FLAGS "-Wall -Wwrite-strings -Wsign-compare ${CMAKE_C_FLAGS}")
	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wno-unused-parameter -Wno-empty-body")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat-security -std=gnu99")
	#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow -D_FORTIFY_SOURCE=2 -Werror")
        set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wtautological-compare -Wno-unused-parameter ${CMAKE_CXX_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCC)

# -Wpointer-bool-conversion is not in GCC yet (as of 10.2)
if (CMAKE_CXX_COMPILER_ID MATCHES ".*clang")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpointer-bool-conversion -Wtautological-compare -Wno-unused-parameter ${CMAKE_CXX_FLAGS}")
endif()

# Building Previous w/o optimization is no fun...
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_C_FLAGS "-O ${CMAKE_C_FLAGS}")
ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug")

# ####################
# Paths configuration:
# ####################

if(NOT BINDIR)
	set(BINDIR bin)
endif()

if(NOT DATADIR)
	set(DATADIR share/previous)
endif()

if(NOT BIN2DATADIR)
	if(WIN32)
		set(BIN2DATADIR "."
		    CACHE STRING "Relative path from bindir to datadir")
	elseif(ENABLE_OSX_BUNDLE)
		set(BIN2DATADIR "../Resources"
		    CACHE STRING "Relative path from bindir to datadir")
	else()
		set(BIN2DATADIR "../share/hatari"
		    CACHE STRING "Relative path from bindir to datadir")
	endif(WIN32)
	mark_as_advanced(BIN2DATADIR)
endif()

if(NOT MANDIR)
	set(MANDIR share/man/man1)
endif()

if(NOT DOCDIR)
	set(DOCDIR share/doc/previous)
endif()

# #########################################
# Create config.h and recurse into subdirs:
# #########################################

configure_file(${CMAKE_SOURCE_DIR}/cmake/config-cmake.h
		${CMAKE_BINARY_DIR}/config.h)

add_subdirectory(src)
