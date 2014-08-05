#include "Gameboy.hpp"
#include "Utils.hpp"
#include <stdio.h>
#include <stdarg.h>

void Gameboy::run()
{
    while (!cpu.isHalted())
        cpu.tick();
}

void Gameboy::memAccess(Word address, Byte* pData, bool isWrite)
{
    if (address <= 0x7fff)
        return rom.memAccess(address, pData, isWrite);
}

Byte Gameboy::memRead8(Word address)
{
    Byte value = 0;
    memAccess(address, &value, false);
    return value;
}

void Gameboy::memWrite8(Word address, Byte value)
{
    memAccess(address, &value, true);
}

Word Gameboy::memRead16(Word address)
{
    return memRead8(address) | (memRead8(address + 1) << 8);
}

void Gameboy::memWrite16(Word address, Word value)
{
    memWrite8(address, (Byte)(value));
    memWrite8(address + 1, (Byte)(value >> 8));
}

void Gameboy::logInsn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    printf("[insn] ");
    vprintf(fmt, ap);
    putchar('\n');

    va_end(ap);
}

void Gameboy::warn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    printf("[warn] ");
    vprintf(fmt, ap);
    putchar('\n');

    va_end(ap);
}
