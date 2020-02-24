cmake_minimum_required (VERSION 3.10.2)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

find_package(PkgConfig)
pkg_check_modules(PC_JSON-C QUIET json-c)

find_path(JSON-C_INCLUDE_DIR json.h
	HINTS ${PC_JSON-C_INCLUDEDIR} ${PC_JSON-C_INCLUDE_DIRS} PATH_SUFFIXES json-c json)

find_library(JSON-C_LIBRARY NAMES json-c libjson-c
	HINTS ${PC_JSON-C_LIBDIR} ${PC_JSON-C_LIBRARY_DIRS})

set(JSON-C_LIBRARIES ${JSON-C_LIBRARY})
set(JSON-C_INCLUDE_DIRS ${JSON-C_INCLUDE_DIR})

include_directories(${JSON-C_INCLUDE_DIRS})

find_library(LibNL_LIBRARY NAMES nl nl-3)
find_library(LibNL_ROUTE_LIBRARY NAMES nl-route nl-route-3)
find_library(LibNL_NETFILTER_LIBRARY NAMES nl-nf nl-nf-3)
find_library(LibNL_GENL_LIBRARY NAMES nl-genl nl-genl-3)

add_executable(px-network-inspection main.c)
target_link_libraries(px-network-inspection ${JSON-C_LIBRARIES})
