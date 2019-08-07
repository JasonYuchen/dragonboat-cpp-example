add_executable(dragonboat_cpp_helloworld
        ../utils/utils.cpp
        statemachine.cpp
        main.cpp)

target_link_libraries(dragonboat_cpp_helloworld
        dragonboatcpp
        dragonboat
        pthread)