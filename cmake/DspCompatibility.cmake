# MSVC accepts a dependent zero-valued integer template argument as nullptr,
# making writeMem<Inst, MMM> ambiguous with its two-argument SFINAE overload.
# Explicitly pass the third template argument to select the same implementation
# chosen by Clang/GCC. Keep this change in a build-local dependency source copy.
if(MSVC)
    set(SILVER_DSP_PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/private-dsp")
    if(NOT EXISTS "${SILVER_DSP_PRIVATE}/CMakeLists.txt")
        file(COPY "${DSP_SOURCE}/" DESTINATION "${SILVER_DSP_PRIVATE}" PATTERN ".git" EXCLUDE)
    endif()
    set(DSP_SOURCE "${SILVER_DSP_PRIVATE}")
    set(move_source "${DSP_SOURCE}/source/dsp56kEmu/dsp_ops_move.inl")
    file(READ "${move_source}" move_text)
    string(REPLACE "writeMem<Inst, MMM>(op, Area, r);"
        "writeMem<Inst, MMM, nullptr>(op, Area, r);" move_text "${move_text}")
    file(WRITE "${move_source}" "${move_text}")
endif()
