cmake_minimum_required(VERSION 3.10)

project(h8s)

# Hitachi/Renesas H8S core. Header only, so this is an interface target that
# exists to hand out the include root. More than one manufacturer's devices use
# this core, which is why it lives under cpu/ rather than under a single one.

set(SOURCES
	h8s.hpp
	h8sdevices.hpp
)

add_library(h8s INTERFACE)

target_include_directories(h8s INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# interface targets carry no sources of their own, this is only so the headers
# show up in IDE project trees
add_custom_target(h8sHeaders SOURCES ${SOURCES})
source_group("source" FILES ${SOURCES})

set_property(TARGET h8sHeaders PROPERTY FOLDER "cpu")

if(BUILD_TESTING)
    add_executable(h8sBit_test h8sBit_test.cpp)
    target_link_libraries(h8sBit_test PRIVATE h8s)
    target_compile_features(h8sBit_test PRIVATE cxx_std_17)
    add_test(NAME h8sBit COMMAND h8sBit_test)
    add_executable(h8sInterruptMode2_test h8sInterruptMode2_test.cpp)
    target_link_libraries(h8sInterruptMode2_test PRIVATE h8s)
    target_compile_features(h8sInterruptMode2_test PRIVATE cxx_std_17)
    add_test(NAME h8sInterruptMode2 COMMAND h8sInterruptMode2_test)
endif()
