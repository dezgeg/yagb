#pragma once
#include "Irq.hpp"
#include "Platform.hpp"

#include <cstring>

class Gpu;
class Logger;
class Rom;
class Timer;

class Bus
{
    Logger* log;
    Rom* rom;
    Gpu* gpu;
    Timer* timer;

    bool bootromEnabled;

    Byte joypadLatches;
    IrqSet irqsEnabled;
    IrqSet irqsPending;

    Byte ram[8192];
    Byte hram[127];

    void joypadAccess(Byte* pData, bool isWrite);
    void memAccess(Word address, Byte* pData, bool isWrite);
    void disableBootrom();

public:
    Bus(Logger* log, Rom* rom, Gpu* gpu, Timer* timer) :
        log(log),
        rom(rom),
        gpu(gpu),
        timer(timer),
        bootromEnabled(true),
        joypadLatches(0x3),
        irqsEnabled(0),
        irqsPending(0)
    {
        std::memset(ram, 0xAA, sizeof(ram));
        std::memset(hram, 0xAA, sizeof(ram));
    }

    Byte memRead8(Word address);
    void memWrite8(Word address, Byte value);
    Word memRead16(Word address);
    void memWrite16(Word address, Word value);

    void raiseIrq(IrqSet irqs);
    void ackIrq(Irq irq);
    IrqSet getEnabledIrqs();
    IrqSet getPendingIrqs();
};
