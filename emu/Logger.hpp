#pragma once

union Regs;
class Logger
{
    long currentCycle;
    int currentScanline;

public:
    void setTimestamp(long cycle, int scanline)
    {
        currentCycle = cycle;
        currentScanline = scanline;
    }

    void logInsn(Regs* regs, int cycles, const char* fmt, ...);
    void warn(const char* fmt, ...);
};
