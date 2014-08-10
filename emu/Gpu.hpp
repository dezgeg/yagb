#pragma once
#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

#include <cstring>

enum {
    ScreenWidth = 160,
    ScreenHeight = 144,
};

class Gpu
{
    Logger* log;

    long frame;
    int cycleResidue;
    Byte framebuffer[ScreenHeight][ScreenWidth];

    Byte vram[8192];
    struct GpuRegs
    {
        Byte lcdc;
        Byte scy;
        Byte scx;
        Byte ly;
        Byte lyc;
        Byte bgp;
        Byte wy;
        Byte wx;
    } regs;

    void renderScanline();

public:
    Gpu(Logger* log) :
        log(log),
        frame(0),
        cycleResidue(0)
    {
        std::memset(&framebuffer[0][0], 0, sizeof(framebuffer));
        std::memset(&vram[0], 0, sizeof(vram));
        std::memset(&regs, 0, sizeof(regs));
    }

    int getCurrentScanline() { return regs.ly; }
    int getCurrentFrame() { return frame; }
    Byte* getFramebuffer() { return &framebuffer[0][0]; }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    void tick(long cycles);
};
