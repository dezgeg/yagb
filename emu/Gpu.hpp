#pragma once
#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

class Gpu
{
    Logger* log;

    int cycleResidue;

    Byte vram[8192];
    Byte lcdc;
    Byte scy;
    Byte scx;
    Byte ly;
    Byte lyc;
    Byte bgp;
    Byte wy;
    Byte wx;

public:
    Gpu(Logger* log) :
        log(log),
        cycleResidue(0)
    {
    }

    int getCurrentScanline() { return ly; }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    void tick(long cycles);
};
