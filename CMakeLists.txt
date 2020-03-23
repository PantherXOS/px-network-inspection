cmake_minimum_required (VERSION 3.10.2)

project(px-network-inspection)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

find_package(PkgConfig)
#pkg_check_modules(PC_JSON-C json-c)
pkg_search_module(GLIB REQUIRED glib-2.0)
find_package (CURL REQUIRED)

message(${CURL_INCLUDE_DIRS})

find_path(JSON-C_INCLUDE_DIR json.h PATH_SUFFIXES json-c json)

find_path(LIBNL_INCLUDE_DIR netlink/netlink.h netlink/object.h netlink/route/link.h PATH_SUFFIXES libnl3)

find_library(JSON-C_LIBRARY NAMES json-c libjson-c)
find_library(LIBNL_LIBRARY NAMES nl nl-3)
find_library(LIBNL_ROUTE_LIBRARY NAMES nl-route nl-route-3)
find_library(LIBNL_NETFILTER_LIBRARY NAMES nl-nf nl-nf-3)
find_library(LIBNL_GENL_LIBRARY NAMES nl-genl nl-genl-3)
find_library(LIBNL_CLI_LIBRARY NAMES nl-cli nl-cli-3)

set(LOCAL_INCLUDE "./include")

include_directories(${JSON-C_INCLUDE_DIR} ${LIBNL_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS} ${LOCAL_INCLUDE}
	${CURL_INCLUDE_DIRS})

set(SRC "main.c" "ethtool-info.c" "gnode-object.c" "route-tree.c" "public-ip.c" "app-profile.c")

#add_executable(px-network-inspection main.c routing-info.c)
add_executable(px-network-inspection ${SRC})
target_link_libraries(px-network-inspection ${JSON-C_LIBRARY} ${LIBNL_LIBRARY} ${LIBNL_ROUTE_LIBRARY}
	${LIBNL_NETFILTER_LIBRARY} ${LIBNL_GENL_LIBRARY} ${GLIB_LDFLAGS} ${LIBNL_CLI_LIBRARY} ${CURL_LIBRARIES})

install(TARGETS px-network-inspection DESTINATION bin COMPONENT binaries)
