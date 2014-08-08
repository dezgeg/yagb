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

void Gameboy::logInsn(Regs* regs, const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[insn] 0x%04X: %-32s "
           "A: 0x%02x | BC: 0x%04x | DE: 0x%04x | HL: 0x%04x | SP: 0x%04x | Flags: %c%c%c%c\n",
           regs->pc, buf, regs->a, regs->bc, regs->de, regs->hl, regs->sp,
           regs->flags.z ? 'Z' : '-', regs->flags.n ? 'N' : '-',
           regs->flags.h ? 'H' : '-', regs->flags.c ? 'C' : '-');
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
