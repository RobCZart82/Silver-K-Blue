#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace kstation
{
    // Firmware is supplied by the user. No ROM bytes are compiled into the plugin.
    class Rom
    {
    public:
        Rom() = default;
        explicit Rom(const std::string& filename);
        explicit Rom(std::vector<uint8_t> bytes, std::string name = {});

        bool isValid() const { return !m_image.empty(); }
        const std::vector<uint8_t>& data() const { return m_image; }
        const std::string& getFilename() const { return m_filename; }
        const std::string& error() const { return m_error; }
        static constexpr size_t ImageSize = 49728;

    private:
        void decode(const std::vector<uint8_t>& bytes);
        std::vector<uint8_t> m_image;
        std::string m_filename;
        std::string m_error = "K-Station OS 2.1.01 (.syx, .os or decoded .bin) is required.";
    };

    class RomLoader
    {
    public:
        static Rom findROM();
    };
}
