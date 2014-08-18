#include "Bus.hpp"
#include "Cpu.hpp"
#include "Logger.hpp"

#include <stdio.h>
#include <stdarg.h>

void Logger::logInsn(Bus* bus, Regs* regs, int cycles, Word newPC, const char* fmt, ...)
{
    if (!insnLoggingEnabled || bus->isBootromEnabled())
        return;

    char hexdumpBuf[sizeof("01 02 03")];
    unsigned insnCount = (Word)(newPC - regs->pc);
    assert(insnCount <= 3);
    for (unsigned i = 0; i < insnCount; i++)
        sprintf(&hexdumpBuf[i * 3], "%02X ", bus->memRead8(regs->pc + i, true));
    hexdumpBuf[3 * insnCount - 1] = 0;

    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[insn %05ld/%03d/%08ld] 0x%04X: %8s => %-32s "
           "A: 0x%02x | BC: 0x%04x | DE: 0x%04x | HL: 0x%04x | SP: 0x%04x | Flags: %c%c%c%c%c | Cycles: %d\n",
           currentFrame, currentScanline, currentCycle,
           regs->pc, hexdumpBuf, buf, regs->a, regs->bc, regs->de, regs->hl, regs->sp,
           regs->flags.z ? 'Z' : '-', regs->flags.n ? 'N' : '-',
           regs->flags.h ? 'H' : '-', regs->flags.c ? 'C' : '-',
           regs->irqsEnabled ? '!' : '.',
           cycles);
}

void Logger::logMemoryAccess(Word addr, Byte data, bool isWrite)
{
    if (!insnLoggingEnabled)
        return;
    printf("[mem %s] 0x%04x: %02x\n", isWrite ? "write" : "read", addr, data);
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
