#include "ksRom.h"

#include <fstream>
#include "baseLib/sha256.h"


#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace ks
{
    namespace
    {
        using Bytes = std::vector<uint8_t>;
        void require(bool condition, const char* message)
        {
            if (!condition) throw std::runtime_error(message);
        }

        uint64_t base128(const uint8_t* bytes, size_t count)
        {
            uint64_t result = 0;
            for (size_t i = 0; i < count; ++i)
            {
                require(bytes[i] < 128, "Invalid seven-bit update field.");
                result |= uint64_t(bytes[i]) << (7 * i);
            }
            return result;
        }

        std::vector<Bytes> frames(const Bytes& input)
        {
            std::vector<Bytes> result;
            const bool sysex = input.front() == 0xf0;
            size_t position = 0;
            while (position < input.size())
            {
                size_t end;
                if (sysex)
                {
                    require(input[position] == 0xf0, "Missing SysEx start.");
                    const auto it = std::find(input.begin() + position + 1, input.end(), 0xf7);
                    require(it != input.end(), "Unterminated SysEx update.");
                    end = size_t(it - input.begin()) + 1;
                }
                else
                {
                    require(position + 2 <= input.size(), "Truncated .OS frame length.");
                    const auto count = size_t(input[position]) | (size_t(input[position + 1]) << 8);
                    position += 2;
                    require(count && count <= input.size() - position, "Truncated .OS update frame.");
                    end = position + count;
                }
                result.emplace_back(input.begin() + position, input.begin() + end);
                position = end;
            }
            return result;
        }

        Bytes decodeUpdate(const Bytes& input)
        {
            const auto messages = frames(input);
            require(messages.size() == 197, "Unsupported KS update structure.");
            constexpr std::array<uint8_t, 7> prefix{0xf0, 0, 0x20, 0x29, 1, 0x42, 0x7f};
            for (const auto& message : messages)
            {
                require(message.size() >= 18 && message.back() == 0xf7,
                        "Invalid KS update frame.");
                require(std::equal(prefix.begin(), prefix.end(), message.begin()),
                        "This firmware is not for KS.");
                require(std::all_of(message.begin() + 1, message.end() - 1,
                                    [](uint8_t v) { return v < 128; }), "Invalid SysEx payload.");
                require(message[9] == 0x11 && message[10] == 2,
                        "Only the verified KS OS 2.1.02 is supported.");
            }
            const auto& start = messages.front();
            const auto& finish = messages.back();
            require(start.size() == 31 && start[7] == 0x7f && finish.size() == 29 && finish[7] == 0x7d,
                    "Invalid update start/end.");
            require(base128(start.data() + 13, 2) == 195 &&
                    base128(start.data() + 15, 3) == Rom::ImageSize, "Invalid update size.");
            Bytes image;
            image.reserve(Rom::ImageSize);
            uint64_t total = 0;
            for (size_t block = 0; block < 195; ++block)
            {
                const auto& message = messages[block + 1];
                require(message.size() == 540 && message[7] == 0x7e, "Invalid data block.");
                require(base128(message.data() + 13, 2) == block, "Update blocks are out of sequence.");
                require(message[15] == 0x20 && message[16] == 0 &&
                        std::all_of(message.begin() + 534, message.begin() + 539,
                                    [](uint8_t v) { return v == 0; }), "Invalid data block metadata.");
                std::array<uint8_t, 448> unpacked{};
                uint32_t accumulator = 0;
                unsigned bits = 0;
                size_t output = 0;
                for (size_t i = 17; i < 529; ++i)
                {
                    accumulator |= uint32_t(message[i]) << bits;
                    bits += 7;
                    if (bits >= 8)
                    {
                        unpacked[output++] = uint8_t(accumulator);
                        accumulator >>= 8;
                        bits -= 8;
                    }
                }
                require(output == unpacked.size() && bits == 0, "Invalid packed block size.");
                uint64_t sum = 0;
                for (size_t i = 0; i < unpacked.size(); i += 2)
                {
                    sum += uint16_t(unpacked[i]) | (uint16_t(unpacked[i + 1]) << 8);
                    image.push_back(unpacked[i + 1]);
                    image.push_back(unpacked[i]);
                }
                require(sum == base128(message.data() + 529, 5), "Update block checksum mismatch.");
                total += sum;
            }
            require(total == base128(finish.data() + 13, 5), "Update checksum mismatch.");
            return image;
        }
    }

    Rom::Rom(const std::string& filename) : m_filename(filename)
    {
        std::ifstream f(filename,std::ios::binary);
        Bytes bytes((std::istreambuf_iterator<char>(f)),{});
        decode(bytes);
    }

    Rom::Rom(Bytes bytes, std::string name) : m_filename(std::move(name)) { decode(bytes); }

    void Rom::decode(const Bytes& bytes)
    {
        try
        {
            require(!bytes.empty() && bytes.size() <= 131072, "Empty or oversized firmware file.");
            auto image = bytes.size() == ImageSize ? bytes : decodeUpdate(bytes);
            constexpr baseLib::SHA256 supported("7acd2d1a8a56f44522b12327437a27826b64fc49a18851816423546fd6ea085c");
            require(baseLib::SHA256(image) == supported,
                    "Unrecognized or damaged firmware; use KS OS 2.1.02.");
            m_image = std::move(image);
            m_error.clear();
        }
        catch (const std::exception& e)
        {
            m_image.clear();
            m_error = e.what();
        }
    }

    Rom RomLoader::findROM(){return {};}
}
