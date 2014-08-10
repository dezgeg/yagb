#pragma once
#include "Platform.hpp"

class Gpu;
class Logger;
class Rom;

class Bus
{
    Logger* log;
    Rom* rom;
    Gpu* gpu;

    Byte ram[8192];
    Byte hram[127];

    void memAccess(Word address, Byte* pData, bool isWrite);

public:
    Bus(Logger* log, Rom* rom, Gpu* gpu) :
        log(log),
        rom(rom),
        gpu(gpu)
    {
    }

    Byte memRead8(Word address);
    void memWrite8(Word address, Byte value);
    Word memRead16(Word address);
    void memWrite16(Word address, Word value);
};
