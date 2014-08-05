#pragma once
#include <cstdint>

typedef uint8_t Byte;
typedef uint16_t Word;

union Regs
{
    struct {
        Byte a, f, b, c, d, e, h, l;
    };
    struct {
        Word af, bc, de, hl, sp, pc;
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
