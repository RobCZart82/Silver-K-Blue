# Silver-K Blue application icon

`Silver-K-Blue-icon.png` is Robert's supplied `ikon.png`, preserved without cropping or retouching. The full interface and keyboard are intentional. JUCE generates the native macOS ICNS and Windows ICO resources from this source through `ICON_BIG` in the plugin CMake configuration. Both existing platform workflows therefore include the icon automatically on their next build.

At small Dock/taskbar sizes, interface text will not be readable; the full image is retained as requested. No cloud build is needed to store this change. Existing released binaries remain unchanged until rebuilt.
