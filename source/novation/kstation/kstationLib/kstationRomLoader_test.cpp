#include "kstationRomLoader.h"
#include "baseLib/filesystem.h"
#include "synthLib/romLoader.h"

#include <iostream>
#include <stdexcept>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
}

int main(int argc, char** argv)
{
    try
    {
        check(argc == 4, "Usage: kstationRomLoader_test decoded.bin update.syx update.os");
        const kstation::Rom raw(argv[1]), syx(argv[2]), os(argv[3]);
        check(raw.isValid(), raw.error().c_str());
        check(syx.isValid(), syx.error().c_str());
        check(os.isValid(), os.error().c_str());
        check(raw.data() == syx.data() && raw.data() == os.data(), "Formats decode differently");

        auto bytes = raw.data();
        bytes[0x84f8] ^= 1;
        check(!kstation::Rom(bytes).isValid(), "Corrupted DSP firmware accepted");
        bytes.pop_back();
        check(!kstation::Rom(bytes).isValid(), "Truncated image accepted");
        check(!kstation::Rom(std::vector<uint8_t>{}).isValid(), "Empty image accepted");

        baseLib::filesystem::readFile(bytes, argv[2]);
        auto altered = bytes;
        altered[31 + 17] ^= 1;
        const kstation::Rom badChecksum(altered);
        check(!badChecksum.isValid() && badChecksum.error().find("checksum") != std::string::npos,
              "Packed block checksum not checked");
        altered = bytes;
        altered[5] = 0x42;
        check(!kstation::Rom(altered).isValid(), "KS firmware header accepted");
        altered = bytes;
        altered[31 + 13] = 1;
        check(!kstation::Rom(altered).isValid(), "Out-of-order blocks accepted");
        altered = bytes;
        altered.back() = 0;
        check(!kstation::Rom(altered).isValid(), "Unterminated update accepted");

        synthLib::RomLoader::setSearchPath(baseLib::filesystem::getPath(argv[2]));
        check(kstation::RomLoader::findROM().isValid(), "Common ROM folder lookup failed");
        std::cout << "K-Station ROM: three formats identical; corruption, truncation, model, sequence and lookup checks passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
