add_executable(dragonboat_cpp_ioservice
        ../utils/utils.cpp
        statemachine.cpp
        ioservice.cpp
        main.cpp)

target_link_libraries(dragonboat_cpp_ioservice
        dragonboatcpp
        dragonboat
        pthread)