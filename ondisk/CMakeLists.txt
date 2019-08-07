add_executable(dragonboat_cpp_ondisk
        ../utils/utils.cpp
        statemachine.cpp
        zupply.cpp
        main.cpp)

target_link_libraries(dragonboat_cpp_ondisk
        dragonboatcpp
        dragonboat
        pthread
        rocksdb)