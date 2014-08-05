#pragma once
#include <cstdint>

typedef uint8_t Byte;
typedef uint16_t Word;

union Regs
{
    struct {
        // This order is dictated by many opcodes (e.g 4x..6x). Note order 'fa', not 'af'.
        Byte b, c, d, e, h, l, f, a;
    };
    struct {
        Word bc, de, hl, sp, fa, pc;
    };
    struct {
        Word : 8 + 4;
        Byte c : 1;     // Carry
        Byte h : 1;     // Half-carry
        Byte n : 1;     // Add/Sub (for BCD operations)
        Byte z : 1;     // Zero
    } flags;
    Byte bytes[8];
    Word words[6];
};

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error "You are using piss-poor hardware and are a loser; give up and get a real job."
#endif

class Gameboy;
class Cpu
{
    Gameboy* gb;
    Regs regs;
    bool halted;

    Byte doAddSub(unsigned lhs, unsigned rhs, bool isSub, bool withCarry);
    void executeInsn_4x_6x(Byte opc);
    void executeInsn_7x_Bx(Byte opc);

public:
    Cpu(Gameboy* gb) :
        gb(gb)
    {
        reset();
    }

    void reset();
    void tick();
};
