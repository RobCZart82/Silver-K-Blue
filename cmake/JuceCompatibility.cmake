# Platform compatibility: JUCE 7 VST3 processing modes and macOS window capture.
# Patch a build-local source copy; never modify an external dependency checkout.
if(APPLE OR WIN32)
    set(SILVER_JUCE_PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/private-juce")
    if(NOT EXISTS "${SILVER_JUCE_PRIVATE}/CMakeLists.txt")
        file(COPY "${JUCE_SOURCE}/" DESTINATION "${SILVER_JUCE_PRIVATE}" PATTERN ".git" EXCLUDE)
    endif()
    set(JUCE_SOURCE "${SILVER_JUCE_PRIVATE}")
    # VST3 kPrefetch requests realtime-quality audio at an arbitrary cadence,
    # including faster than realtime. Use the existing full-speed render path
    # so the emulation queue is not drained as if each call had a live deadline.
    # kRealtime keeps its nonblocking queue-consumer path. This changes no DSP
    # quality, guest clock, queue size, latency, or worker priority.
    set(vst3_source "${JUCE_SOURCE}/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp")
    set(SILVER_JUCE_VST3_SOURCE "${vst3_source}")
    include("${CMAKE_CURRENT_LIST_DIR}/PatchJuceVst3Prefetch.cmake")
endif()

if(APPLE)
    set(snapshot_source "${JUCE_SOURCE}/modules/juce_gui_basics/native/juce_Windowing_mac.mm")
    file(READ "${snapshot_source}" snapshot_text)
    string(FIND "${snapshot_text}" "CGImageRef screenShot = CGWindowListCreateImage" legacy_position)
    if(NOT legacy_position EQUAL -1)
        string(REGEX REPLACE "CGImageRef screenShot = CGWindowListCreateImage \\(CGRectNull,[^;]+;"
            "NSView* view = [nsWindow contentView];\n        NSBitmapImageRep* bitmapRep = [[view bitmapImageRepForCachingDisplayInRect: [view bounds]] retain];\n        [view cacheDisplayInRect: [view bounds] toBitmapImageRep: bitmapRep];" snapshot_text "${snapshot_text}")
        string(REPLACE "NSBitmapImageRep* bitmapRep = [[NSBitmapImageRep alloc] initWithCGImage: screenShot];" "" snapshot_text "${snapshot_text}")
        string(REPLACE "CGImageRelease (screenShot);" "" snapshot_text "${snapshot_text}")
        file(WRITE "${snapshot_source}" "${snapshot_text}")
    endif()
endif()
