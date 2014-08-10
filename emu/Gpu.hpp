#pragma once
#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

class Gpu
{
    Logger* log;

    long frame;
    int cycleResidue;
    Byte framebuffer[144][160]; // Order Y-X

    Byte vram[8192];
    Byte lcdc;
    Byte scy;
    Byte scx;
    Byte ly;
    Byte lyc;
    Byte bgp;
    Byte wy;
    Byte wx;

    void renderScanline();

public:
    Gpu(Logger* log) :
        log(log),
        frame(0),
        cycleResidue(0)
    {
    }

    int getCurrentScanline() { return ly; }
    int getCurrentFrame() { return frame; }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    void tick(long cycles);
};
