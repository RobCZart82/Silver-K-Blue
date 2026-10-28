#include "h8s.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

// Exercise the decoder, memory addressing and CCR, including register indices
// with all high bits set. H8 bit numbers always use only Rn[2:0].
int main()
{
    try
    {
        auto cpu = std::make_unique<H8SEmulator>();
        constexpr int codeAddress = 0x1000;
        unsigned cases = 0;
        for (unsigned form = 0; form < 5; ++form)
        for (unsigned op = 0x60; op <= 0x63; ++op)
        for (unsigned index = 0; index < 256; ++index)
        for (unsigned initial : {0u, 0x55u, 0xaau, 0xffu})
        {
            const bool test = op == 0x63;
            const unsigned prefix = test ? 0x7c : 0x7d;
            unsigned target = 0x7000;
            std::vector<uint8_t> code;
            switch (form)
            {
            case 0: code = {uint8_t(op), 0xd8}; break; // R5L, R0L
            case 1: code = {uint8_t(prefix), 0x10, uint8_t(op), 0xd0}; break; // @ER1
            case 2: target = 0xffffb0; code = {uint8_t(prefix + 2), 0xb0, uint8_t(op), 0xd0}; break;
            case 3: target = 0xff8000; code = {0x6a, uint8_t(test ? 0x10 : 0x18), 0x80, 0, uint8_t(op), 0xd0}; break;
            case 4: code = {0x6a, uint8_t(test ? 0x30 : 0x38), 0, 0, 0x70, 0, uint8_t(op), 0xd0}; break;
            }
            cpu->loadmem(code.data(), unsigned(code.size()), codeAddress);
            for (auto& reg : cpu->regs) reg.er = 0;
            cpu->reg32(1) = 0x7000;
            cpu->reg8(13) = uint8_t(index);
            if (form == 0) cpu->reg8(8) = uint8_t(initial);
            else cpu->memory[target] = uint8_t(initial);
            cpu->memory[target - 1] = 0x39;
            cpu->memory[target + 1] = 0x73;
            cpu->ccr = 0xa9;
            cpu->pc = cpu->makepc(codeAddress);
            cpu->step();
            const unsigned mask = 1u << (index & 7);
            const unsigned expected = op == 0x60 ? (initial | mask) :
                                      op == 0x61 ? (initial ^ mask) :
                                      op == 0x62 ? (initial & ~mask) : initial;
            const unsigned actual = form == 0 ? uint8_t(cpu->reg8(8)) : cpu->memory[target];
            const unsigned expectedCcr = test ? (0xa9 | ((initial & mask) ? 0 : h8state::ccr_z)) : 0xa9;
            if (actual != (expected & 255) || cpu->ccr != expectedCcr ||
                cpu->getPC() != codeAddress + int(code.size()) || uint8_t(cpu->reg8(13)) != index ||
                cpu->memory[target - 1] != 0x39 || cpu->memory[target + 1] != 0x73)
            {
                std::cerr << "form=" << form << " op=" << op << " index=" << index
                          << " initial=" << initial << " actual=" << actual << " expected=" << (expected & 255) << '\n';
                throw std::runtime_error("H8 register bit-index semantics failed");
            }
            ++cases;
        }
        std::cout << "H8 bit-index cases passed: " << cases << '\n';
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
