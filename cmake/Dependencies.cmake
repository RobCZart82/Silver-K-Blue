include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
# Exact revisions used by Silver-K 0.1.0. Optional *_SOURCE paths are source
# checkouts only: the build always compiles dependencies, never imports binaries.
function(silver_dependency name repo revision path_variable)
    set(${path_variable} "" CACHE PATH "Optional local ${name} source checkout")
    if(NOT ${path_variable})
        FetchContent_Declare(${name}
            URL "https://codeload.github.com/${repo}/tar.gz/${revision}"
            SOURCE_SUBDIR silver-download-only)
        FetchContent_MakeAvailable(${name})
        set(${path_variable} "${${name}_SOURCE_DIR}" PARENT_SCOPE)
    else()
        if(NOT EXISTS "${${path_variable}}/CMakeLists.txt")
            message(FATAL_ERROR "Invalid ${path_variable}: source CMakeLists.txt missing")
        endif()
    endif()
endfunction()
silver_dependency(silver_juce dsp56300/JUCE 2a8699d723fe9279e1148275485ddb0e18957f56 JUCE_SOURCE)
silver_dependency(silver_asmjit dsp56300/asmjit 3577608cab0bc509f856ebf6e41b2f9d9f71acc4 ASMJIT_SOURCE)
silver_dependency(silver_dsp dsp56300/dsp56300 44353be2f0156cac970c6ab4f467ea1998b40605 DSP_SOURCE)
