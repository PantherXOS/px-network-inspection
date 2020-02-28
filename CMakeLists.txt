cmake_minimum_required (VERSION 3.10.2)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

find_package(PkgConfig)
pkg_check_modules(PC_JSON-C json-c)

find_path(JSON-C_INCLUDE_DIR json.h
	HINTS ${PC_JSON-C_INCLUDEDIR} ${PC_JSON-C_INCLUDE_DIRS} PATH_SUFFIXES json-c json)

find_path(LIBNL_INCLUDE_DIR netlink/netlink.h netlink/route/link.h
	/usr/include
	/usr/include/libnl3
	/usr/local/include
	/usr/local/include/libnl3
)

find_library(JSON-C_LIBRARY NAMES json-c libjson-c
	HINTS ${PC_JSON-C_LIBDIR} ${PC_JSON-C_LIBRARY_DIRS})

find_library(LIBNL_LIBRARY NAMES nl nl-3)
find_library(LIBNL_ROUTE_LIBRARY NAMES nl-route nl-route-3)
find_library(LIBNL_NETFILTER_LIBRARY NAMES nl-nf nl-nf-3)
find_library(LIBNL_GENL_LIBRARY NAMES nl-genl nl-genl-3)

set(JSON-C_LIBRARIES ${JSON-C_LIBRARY})
set(JSON-C_INCLUDE_DIRS ${JSON-C_INCLUDE_DIR})

include_directories(${JSON-C_INCLUDE_DIRS} ${LIBNL_INCLUDE_DIR})

add_executable(px-network-inspection main.c)
target_link_libraries(px-network-inspection ${JSON-C_LIBRARIES} ${LIBNL_LIBRARY} ${LIBNL_ROUTE_LIBRARY} ${LIBNL_NETFILTER_LIBRARY} ${LIBNL_GENL_LIBRARY})
