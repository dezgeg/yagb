#pragma once

union Regs;
class Logger
{
public:
    void logInsn(Regs* regs, const char* fmt, ...);
    void warn(const char* fmt, ...);
};
