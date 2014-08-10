#pragma once

union Regs;
class Logger
{
    long currentCycle;

public:
    void setCycle(long cycle) { currentCycle = cycle; }

    void logInsn(Regs* regs, int cycles, const char* fmt, ...);
    void warn(const char* fmt, ...);
};
