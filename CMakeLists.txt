cmake_minimum_required (VERSION 3.10.2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")

find_library(LibNL_LIBRARY NAMES nl nl-3)
find_library(LibNL_ROUTE_LIBRARY NAMES nl-route nl-route-3)
find_library(LibNL_NETFILTER_LIBRARY NAMES nl-nf nl-nf-3)
find_library(LibNL_GENL_LIBRARY NAMES nl-genl nl-genl-3)

add_executable(px-network-inspection main.c)
