#pragma once
#include "BusUtil.hpp"
#include "Irq.hpp"
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
    Byte oam[0xa0];
    struct GpuRegs
    {
        union {
            Byte lcdc;
            struct {
                Byte bgEnabled : 1;
                Byte objEnabled : 1;
                Byte objSizeLarge : 1;
                Byte bgTileBaseSelect : 1;
                Byte bgPatternBaseSelect : 1;
                Byte winEnabled : 1;
                Byte winPatternBaseSelect : 1;
                Byte lcdEnabled : 1;
            };
        };
        union {
            Byte stat;
            struct {
                Byte mode : 2;
                Byte coincidence : 1;
                Byte hBlankIrqEnabled : 1;
                Byte vBlankIrqEnabled : 1;
                Byte oamIrqEnabled : 1;
                Byte coincidenceIrqEnabled : 1;
            };
        };
        Byte scy;
        Byte scx;
        Byte ly;
        Byte lyc;
        Byte bgp;
        Byte obp0;
        Byte obp1;
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
        std::memset(&oam[0], 0, sizeof(oam));
        std::memset(&regs, 0, sizeof(regs));
    }

    int getCurrentScanline() { return regs.ly; }
    int getCurrentFrame() { return frame; }
    Byte* getFramebuffer() { return &framebuffer[0][0]; }
    Byte* getVram() { return vram; }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void oamAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    Irq tick(long cycles);
};
