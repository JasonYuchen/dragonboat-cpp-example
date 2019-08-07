cmake_minimum_required(VERSION 2.80)
project(dragonboat_cpp_example)

set(CMAKE_CXX_STANDARD 11)

find_library(ROCKSDB_LIBRARY NAMES rocksdb)

include_directories(/usr/local/include)
include_directories(utils)

add_subdirectory(helloworld)
add_subdirectory(ioservice)
add_subdirectory(multigroup)
if(ROCKSDB_LIBRARY)
    add_subdirectory(ondisk)
endif()