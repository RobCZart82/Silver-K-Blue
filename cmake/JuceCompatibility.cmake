# JUCE 7's CGWindowListCreateImage was removed from newer macOS SDKs.
# Patch a build-local source copy; never modify an external dependency checkout.
if(APPLE)
    set(SILVER_JUCE_PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/private-juce")
    if(NOT EXISTS "${SILVER_JUCE_PRIVATE}/CMakeLists.txt")
        file(COPY "${JUCE_SOURCE}/" DESTINATION "${SILVER_JUCE_PRIVATE}" PATTERN ".git" EXCLUDE)
    endif()
    set(JUCE_SOURCE "${SILVER_JUCE_PRIVATE}")
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
