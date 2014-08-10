#include "Cpu.hpp"
#include "Logger.hpp"

#include <stdio.h>
#include <stdarg.h>

void Logger::logInsn(Regs* regs, int cycles, const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[insn %08ld/%03d] 0x%04X: %-32s "
           "A: 0x%02x | BC: 0x%04x | DE: 0x%04x | HL: 0x%04x | SP: 0x%04x | Flags: %c%c%c%c | Cycles: %d\n",
           currentCycle, currentScanline,
           regs->pc, buf, regs->a, regs->bc, regs->de, regs->hl, regs->sp,
           regs->flags.z ? 'Z' : '-', regs->flags.n ? 'N' : '-',
           regs->flags.h ? 'H' : '-', regs->flags.c ? 'C' : '-',
           cycles);
}

void Logger::warn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    printf("[warn] ");
    vprintf(fmt, ap);
    putchar('\n');

    va_end(ap);
}
