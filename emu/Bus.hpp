#pragma once
#include "Irq.hpp"
#include "Platform.hpp"

#include <cstring>

class Gpu;
class Logger;
class Rom;
class Timer;
class Joypad;

class Bus
{
    Logger* log;
    Rom* rom;
    Gpu* gpu;
    Timer* timer;
    Joypad* joypad;

    bool bootromEnabled;
    bool dmaInProgress;
    int dmaCycles;
    Byte dmaSourcePage;

    IrqSet irqsEnabled;
    IrqSet irqsPending;

    Byte ram[8192];
    Byte hram[127];

    void dmaRegAccess(Byte* pData, bool isWrite);
    void memAccess(Word address, Byte* pData, bool isWrite, bool emulatorInternal);
    void disableBootrom();

public:
    Bus(Logger* log, Rom* rom, Gpu* gpu, Timer* timer, Joypad* joypad) :
        log(log),
        rom(rom),
        gpu(gpu),
        timer(timer),
        joypad(joypad),
        bootromEnabled(true),
        dmaInProgress(false),
        dmaCycles(0),
        dmaSourcePage(0),
        irqsEnabled(0),
        irqsPending(0)
    {
        std::memset(ram, 0xAA, sizeof(ram));
        std::memset(hram, 0xAA, sizeof(ram));
    }

    void tickDma(int cycles);

    Byte memRead8(Word address, bool emulatorInternal=false);
    void memWrite8(Word address, Byte value, bool emulatorInternal=false);
    Word memRead16(Word address, bool emulatorInternal=false);
    void memWrite16(Word address, Word value, bool emulatorInternal=false);

    void raiseIrq(IrqSet irqs);
    void ackIrq(Irq irq);
    IrqSet getEnabledIrqs();
    IrqSet getPendingIrqs();

    bool isBootromEnabled();
};
