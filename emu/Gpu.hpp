#pragma once

#include "Bus.hpp"
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

union GbColor {
    struct {
        unsigned r : 5;
        unsigned g : 5;
        unsigned b : 5;
        unsigned isGrayscale : 1;
    }__attribute__((packed));
    struct {
        unsigned dmgGrayscale : 2;
        unsigned pad : 14;
    }__attribute__((packed));

    explicit GbColor() {
        this->isGrayscale = true;
        this->dmgGrayscale = 0;
    }
    explicit GbColor(unsigned grayscale) {
        this->isGrayscale = true;
        this->dmgGrayscale = grayscale;
    }
    explicit GbColor(unsigned r, unsigned g, unsigned b) {
        this->isGrayscale = false;
        this->r = r;
        this->g = g;
        this->b = b;
    }
} __attribute__((packed));
static_assert(sizeof(GbColor) == 2, "GbColor is wrong");

struct OamEntry {
    Byte y;
    Byte x;
    Byte tile;
    union OamFlags {
        Byte byteVal;
        struct {
            Byte unused : 4;
            Byte dmgPalette : 1;
            Byte xFlip : 1;
            Byte yFlip : 1;
            Byte lowPriority : 1;
        };
        struct {
            Byte cgbPalette : 3;
            Byte cgbTileVramBank : 1;
            Byte unused2 : 4;
        };
    } flags;
};
static_assert(sizeof(OamEntry) == 4, "OamEntry is wrong");

union PaletteIndexReg {
    Byte raw;
    struct {
        Byte index : 6;
        Byte unused : 1;
        Byte autoincrement : 1;
    };
};
static_assert(sizeof(PaletteIndexReg) == 1, "PaletteIndexReg is wrong");

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

    Byte vramBank;
    PaletteIndexReg cgbBackgroundPaletteIndex;
    PaletteIndexReg cgbSpritePaletteIndex;
};

class Gpu {
    Logger* log;
    Bus* bus;

    bool renderEnabled;
    long frame;
    int cycleResidue;
    GbColor framebuffer[ScreenHeight][ScreenWidth];
    SByte visibleSprites[40];

    GpuRegs regs;
    Byte vram[16384];
    union {
        Byte oam[0xa0];
        OamEntry sprites[40];
    };
    Byte cgbBackgroundPalette[64];
    Byte cgbSpritePalette[64];

    void renderScanline();
    void captureSpriteState();

    Byte drawTilePixel(Byte* tile, unsigned x, unsigned y, bool large=false,
            OamEntry::OamFlags flags=OamEntry::OamFlags());

public:
    Gpu(Logger* log, Bus* bus) :
            log(log),
            bus(bus),
            renderEnabled(true),
            frame(0),
            cycleResidue(0) {
        std::memset(&framebuffer[0][0], 0, sizeof(framebuffer));
        std::memset(&vram[0], 0, sizeof(vram));
        std::memset(&oam[0], 0, sizeof(oam));
        std::memset(&cgbBackgroundPalette[0], 0, sizeof(cgbBackgroundPalette));
        std::memset(&cgbSpritePalette[0], 0, sizeof(cgbSpritePalette));
        std::memset(&regs, 0, sizeof(regs));
        std::memset(&visibleSprites[0], 0, sizeof(visibleSprites));
    }

    static inline GbColor applyDmgPalette(Byte palette, Byte colorIndex) {
        return GbColor((palette >> colorIndex * 2) & 0x3);
    }

    int getCurrentScanline() { return regs.ly; }
    int getCurrentFrame() { return frame; }
    Word* getFramebuffer() { return (Word*)&framebuffer[0][0]; }
    Byte* getVram() { return vram; }
    GpuRegs* getRegs() { return &regs; }
    void setRenderEnabled(bool renderEnabled) { this->renderEnabled = renderEnabled; }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void oamAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    IrqSet tick(long cycles);
    void serialize(Serializer& ser);
};
