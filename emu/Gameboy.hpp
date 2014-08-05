#include "Cpu.hpp"
#include "Rom.hpp"

class Gameboy
{
    Cpu cpu;
    Rom rom;

    void memAccess(Word address, Byte* pData, bool isWrite);

public:
    Gameboy(const char* romFileName) :
        cpu(this),
        rom(this, romFileName)
    {
    }

    Byte memRead8(Word address);
    void memWrite8(Word address, Byte value);
    Word memRead16(Word address);
    void memWrite16(Word address, Word value);

    void logInsn(const char* fmt, ...);
    void warn(const char* fmt, ...);

    void run();
};
