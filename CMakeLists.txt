cmake_minimum_required(VERSION 2.80)
project(dragonboat_cpp_example)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/include)
include_directories(utils)
aux_source_directory(${EXAMPLE} EXAMPLE_SRC)
aux_source_directory(utils EXAMPLE_SRC)

add_executable(dragonboat_cpp_example ${EXAMPLE_SRC})

if ("${EXAMPLE}" STREQUAL "ondisk")
target_link_libraries(dragonboat_cpp_example
        dragonboatcpp
        dragonboat
        pthread
        rocksdb)
else ()
    target_link_libraries(dragonboat_cpp_example
            dragonboatcpp
            dragonboat
            pthread)
endif ()