cmake_minimum_required(VERSION 3.15)
project(kstationLib)

add_library(kstationLib STATIC
    kstationDevice.cpp kstationDevice.h
    kstationDSP.cpp kstationDSP.h
    kstationH8.cpp kstationH8.h
    kstationHardware.cpp kstationHardware.h
    kstationQueue.h
    kstationRomLoader.cpp kstationRomLoader.h)
target_compile_features(kstationLib PUBLIC cxx_std_17)
target_link_libraries(kstationLib PUBLIC synthLib dsp56kEmu h8s)
target_include_directories(kstationLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
set_property(TARGET kstationLib PROPERTY FOLDER "Gearmulator")

if(BUILD_TESTING)
    add_executable(kstationRomLoader_test kstationRomLoader_test.cpp)
    target_link_libraries(kstationRomLoader_test PRIVATE kstationLib)
    # Firmware is external; run this executable with the three user ROM paths.
    add_executable(kstationQueue_test kstationQueue_test.cpp)
    target_link_libraries(kstationQueue_test PRIVATE kstationLib)
    add_test(NAME kstationQueue COMMAND kstationQueue_test)
endif()

option(KSTATION_BUILD_HARDWARE_CHECK "Build original reset/MIDI/audio firmware check" OFF)
if(KSTATION_BUILD_HARDWARE_CHECK)
    add_executable(kstationHardware_test kstationHardware_test.cpp)
    target_link_libraries(kstationHardware_test PRIVATE kstationLib)
    add_executable(kstationArp_test kstationArp_test.cpp)
    target_link_libraries(kstationArp_test PRIVATE kstationLib)
endif()
