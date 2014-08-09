#pragma once

#include "Cpu.hpp"
#include "Logger.hpp"
#include "Rom.hpp"

class Gameboy
{
    Cpu cpu;
    Rom rom;

    void memAccess(Word address, Byte* pData, bool isWrite);

public:
    Gameboy(Logger* log, const char* romFileName) :
        cpu(this, log),
        rom(log, romFileName)
    {
    }

    Byte memRead8(Word address);
    void memWrite8(Word address, Byte value);
    Word memRead16(Word address);
    void memWrite16(Word address, Word value);

    void run();
};
