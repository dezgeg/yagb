#include "Gpu.hpp"

/*
 * Nocash GPU timings (160x144 display):
 *      Mode 2  2_____2_____2_____2_____2_____2___________________2____ OAM  fetch, 77-83 clks/scanline
 *      Mode 3  _33____33____33____33____33____33__________________3___ VRAM fetch, 169-175 clks/scanline
 *      Mode 0  ___000___000___000___000___000___000________________000 HBlank,     201-207 clks/scanline
 *      Mode 1  ____________________________________11111111111111_____ VBlank,     4560 clks/frame
 *  - Each scanline is 456 clks. I guess 80 + 172 + 204 = 456 can be selected.
 *      => The GPU draws roughly 1 pixel / clock (excluding hblank/per-scanline setup)
 *  - Each frame is 70224 clks (456 * 144 + 4560)
 */

enum {
    OamFetchThresholdCycles = 80,
    VramFetchThresholdCycles = 80 + 172,
    ScanlineCycles = 456,

    MaxScanline = 153,
};

IrqSet Gpu::tick(long cycles)
{
    IrqSet irqs = 0;

    bool wasInOamFetch = cycleResidue < OamFetchThresholdCycles;
    bool wasInHBlank = cycleResidue >= VramFetchThresholdCycles;

    cycleResidue += cycles;

    bool nowInOamFetch = cycleResidue < OamFetchThresholdCycles;
    bool nowInHBlank = cycleResidue >= VramFetchThresholdCycles;

    if (cycleResidue >= ScanlineCycles) {
        if (regs.ly < ScreenHeight)
            renderScanline();

        // Assumes that cycle delta is not insanely large
        cycleResidue -= ScanlineCycles;
        regs.ly++;
        if (regs.ly > MaxScanline) {
            regs.ly = 0;
            frame++;
        } else if (regs.ly == ScreenHeight) {
            irqs |= bit(Irq_VBlank);
            if (regs.vBlankIrqEnabled)
                irqs |= bit(Irq_LcdStat);
        }

        if (regs.ly == regs.lyc && regs.coincidenceIrqEnabled) {
            irqs |= bit(Irq_LcdStat);
        }
    } else {
        if (nowInOamFetch && !wasInOamFetch && regs.oamIrqEnabled) {
            irqs |= bit(Irq_LcdStat);
        }
        if (nowInHBlank && ! wasInHBlank && regs.hBlankIrqEnabled) {
            irqs |= bit(Irq_LcdStat);
        }
    }

    return irqs;
}

void Gpu::renderScanline()
{
    unsigned bgY = regs.ly + regs.scy;
    unsigned bgTileY = (bgY / 8) % 32;
    unsigned bgTileYBit = bgY % 8;

    Byte* bgTileBase = regs.bgTileBaseSelect ? &vram[0x1c00] : &vram[0x1800];    // Bit 3
    Byte* bgPatternBase = regs.bgPatternBaseSelect ? &vram[0x0]: &vram[0x1000];  // Bit 4
    for (unsigned i = 0; i < ScreenWidth; i++) {
        if (!regs.lcdEnabled || !regs.bgEnabled) {
            framebuffer[regs.ly][i] = 0;
            continue;
        }

        unsigned bgX = i + regs.scx;
        unsigned bgTileX = (bgX / 8) % 32;
        unsigned bgTileXBit = bgX % 8;

        Byte tileNum = bgTileBase[bgTileY * 32 + bgTileX];
        long tileOff = regs.bgPatternBaseSelect ? (long)tileNum : (long)(SByte)tileNum;
        Byte lsbs = bgPatternBase[16 * tileOff + 2 * bgTileYBit];
        Byte msbs = bgPatternBase[16 * tileOff + 2 * bgTileYBit + 1];

        unsigned colorIndex = !!(lsbs & (0x80 >> bgTileXBit)) |
                              ((!!(msbs & (0x80 >> bgTileXBit))) << 1);
        framebuffer[regs.ly][i] = (regs.bgp >> colorIndex * 2) & 0x3;
    }
}

void Gpu::vramAccess(Word offset, Byte* pData, bool isWrite)
{
#if 0
    if (isWrite)
        log->warn("GPU VRAM write [0x%0x] = 0x%02x", 0x8000 + offset, *pData);
#endif
    BusUtil::arrayMemAccess(vram, offset, pData, isWrite);
}

void Gpu::oamAccess(Word offset, Byte* pData, bool isWrite)
{
#if 0
    if (isWrite)
        log->warn("GPU OAM write [0x%0x] = 0x%02x", 0xfe00 + offset, *pData);
#endif
    BusUtil::arrayMemAccess(oam, offset, pData, isWrite);
}

void Gpu::registerAccess(Word reg, Byte* pData, bool isWrite)
{
#if 0
    if (isWrite)
        log->warn("GPU reg write [0x%0x] = 0x%02x", reg, *pData);
#endif

    switch (reg) {
        case 0xff40: BusUtil::simpleRegAccess(&regs.lcdc, pData, isWrite); return;
        case 0xff41: {
            if (isWrite) {
                log->warn("STAT write = %02x", *pData);
                regs.stat = *pData & 0xf8;
            } else {
                Byte tmp = regs.stat;
                unsigned mode = cycleResidue >= VramFetchThresholdCycles ? 0 // HBlank
                              : regs.ly >= ScreenHeight ? 1                  // VBlank
                              : cycleResidue >= OamFetchThresholdCycles ? 2  // OAM access
                              : 3;                                           // VRAM access
                tmp |= !!(regs.ly == regs.lyc) << 2;
                tmp |= mode;

                *pData = tmp;
            }
            return;
        }
        case 0xff42: BusUtil::simpleRegAccess(&regs.scy, pData, isWrite); return;
        case 0xff43: BusUtil::simpleRegAccess(&regs.scx, pData, isWrite); return;
        case 0xff44: {
            // XXX: what happens on LY write?
            if (isWrite)
                log->warn("GPU register write to LY");
            else
                *pData = regs.ly;
            return;
        }
        case 0xff45: BusUtil::simpleRegAccess(&regs.lyc, pData, isWrite); return;
        // case 0xff46: dma - TODO: must be handled by Bus!
        case 0xff47: BusUtil::simpleRegAccess(&regs.bgp, pData, isWrite); return;
        case 0xff48: BusUtil::simpleRegAccess(&regs.obp0, pData, isWrite); return;
        case 0xff49: BusUtil::simpleRegAccess(&regs.obp1, pData, isWrite); return;
        case 0xff4a: BusUtil::simpleRegAccess(&regs.wy, pData, isWrite); return;
        case 0xff4b: BusUtil::simpleRegAccess(&regs.wx, pData, isWrite); return;
    }
    log->warn("Unhandled GPU register %s to register %04X", isWrite ? "write" : "read", reg);
}
