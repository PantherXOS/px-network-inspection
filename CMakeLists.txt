cmake_minimum_required (VERSION 3.10.2)

project(px-network-inspection)

option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

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
