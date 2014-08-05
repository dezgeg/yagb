#include "Cpu.hpp"

class Gameboy
{
    Cpu cpu;

public:
    Gameboy() :
        cpu(this)
    {
    }

    Byte memRead8(Word address);
    void memWrite8(Word address, Byte value);

    void logInsn(const char* fmt, ...);
};
