add_executable(dragonboat_cpp_multigroup
        ../utils/utils.cpp
        statemachines.cpp
        main.cpp)

target_link_libraries(dragonboat_cpp_multigroup
        dragonboatcpp
        dragonboat
        pthread)