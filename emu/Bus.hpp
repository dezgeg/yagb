#pragma once

#include "Logger.hpp"
#include "Platform.hpp"
#include "Rom.hpp"

class Bus
{
    Logger* log;
    Rom* rom;

    void memAccess(Word address, Byte* pData, bool isWrite);

public:
    Bus(Logger* log, Rom* rom) :
        log(log),
        rom(rom)
    {
    }

    Byte memRead8(Word address);
    void memWrite8(Word address, Byte value);
    Word memRead16(Word address);
    void memWrite16(Word address, Word value);
};
