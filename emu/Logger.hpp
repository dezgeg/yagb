#pragma once

union Regs;
class Logger
{
    bool insnLoggingEnabled;

    long currentFrame;
    long currentCycle;
    int currentScanline;

public:
    Logger(bool insnLoggingEnabled=false) :
        insnLoggingEnabled(insnLoggingEnabled)
    {
    }

    void setTimestamp(long frame, int scanline, long cycle)
    {
        currentFrame = frame;
        currentCycle = cycle;
        currentScanline = scanline;
    }

    void logInsn(Regs* regs, int cycles, const char* fmt, ...);
    void warn(const char* fmt, ...);
};
