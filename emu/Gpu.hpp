#pragma once

#include "BusUtil.hpp"
#include "Irq.hpp"
#include "Logger.hpp"
#include "Platform.hpp"
#include "Serializer.hpp"

#include <cstring>

enum {
    ScreenWidth = 160,
    ScreenHeight = 144,

    ScanlineCycles = 456,
};

struct OamEntry {
    Byte y;
    Byte x;
    Byte tile;
    union OamFlags {
        Byte byteVal;
        struct {
            Byte unused : 4;
            Byte palette : 1;
            Byte xFlip : 1;
            Byte yFlip : 1;
            Byte lowPriority : 1;
        };
    } flags;
};

struct GpuRegs {
    union {
        Byte lcdc;
        struct {
            Byte bgEnabled : 1;
            Byte objEnabled : 1;
            Byte objSizeLarge : 1;
            Byte bgTileBaseSelect : 1;
            Byte bgPatternBaseSelect : 1;
            Byte winEnabled : 1;
            Byte winTileBaseSelect : 1;
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
};

class Gpu {
    Logger* log;

    bool renderEnabled;
    long frame;
    int cycleResidue;
    Byte framebuffer[ScreenHeight][ScreenWidth];
    SByte visibleSprites[40];

    GpuRegs regs;
    Byte vram[8192];
    union {
        Byte oam[0xa0];
        OamEntry sprites[40];
    };

    void renderScanline();
    void captureSpriteState();

    Byte drawTilePixel(Byte* tile, unsigned x, unsigned y, bool large=false,
            OamEntry::OamFlags flags=OamEntry::OamFlags());

public:
    Gpu(Logger* log) :
            log(log),
            renderEnabled(true),
            frame(0),
            cycleResidue(0) {
        std::memset(&framebuffer[0][0], 0, sizeof(framebuffer));
        std::memset(&vram[0], 0, sizeof(vram));
        std::memset(&oam[0], 0, sizeof(oam));
        std::memset(&regs, 0, sizeof(regs));
        std::memset(&visibleSprites[0], 0, sizeof(visibleSprites));
    }

    static inline Byte applyPalette(Byte palette, Byte colorIndex) {
        return (palette >> colorIndex * 2) & 0x3;
    }

    int getCurrentScanline() { return regs.ly; }
    int getCurrentFrame() { return frame; }
    Byte* getFramebuffer() { return &framebuffer[0][0]; }
    Byte* getVram() { return vram; }
    GpuRegs* getRegs() { return &regs; }
    void setRenderEnabled(bool renderEnabled) { this->renderEnabled = renderEnabled; }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void oamAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    IrqSet tick(long cycles);
    void serialize(Serializer& ser);
};
