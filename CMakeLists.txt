cmake_minimum_required(VERSION 3.10)
project(dragonboat_cpp_example)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/local/include)
include_directories(utils)
aux_source_directory(${EXAMPLE} EXAMPLE_SRC)
aux_source_directory(utils EXAMPLE_SRC)

add_executable(dragonboat_cpp_example ${EXAMPLE_SRC})

target_link_libraries(dragonboat_cpp_example
        dragonboatcpp
        dragonboat
        pthread)