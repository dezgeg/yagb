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

    ScreenWidth = 160,
    ScreenHeight = 144,
    MaxScanline = 153,
};

void Gpu::tick(long cycles)
{
    cycleResidue += cycles;
    if (cycleResidue >= ScanlineCycles) {
        renderScanline();
        // Assumes that cycle delta is not insanely large
        cycleResidue -= ScanlineCycles;
        ly++;
        if (ly > MaxScanline) {
            ly = 0;
            frame++;
        }
    }
}

void Gpu::renderScanline()
{
    unsigned bgY = ly + scy;
    unsigned bgTileY = (bgY / 8) % 32;
    unsigned bgTileYBit = bgY % 8;

    Byte* bgTileBase = &vram[0x1800]; // TODO: LCDC bit
    Byte* bgPatternBase = &vram[0x0]; // TODO: LCDC bit
    for (unsigned i = 0; i < ScreenWidth; i++) {
        unsigned bgX = i + scy;
        unsigned bgTileX = (bgX / 8) % 32;
        unsigned bgTileXBit = bgX % 8;

        Byte tileNum = bgTileBase[bgTileY * 32 + bgTileX];
        Byte lsbs = bgPatternBase[16 * tileNum + 2 * bgTileYBit];
        Byte msbs = bgPatternBase[16 * tileNum + 2 * bgTileYBit + 1];

        framebuffer[ly][i] = !!(lsbs & (1 << bgTileXBit)) | (!!(msbs & (1 << bgTileXBit)) << 1);
    }
}

void Gpu::vramAccess(Word offset, Byte* pData, bool isWrite)
{
    BusUtil::arrayMemAccess(vram, offset, pData, isWrite);
}

void Gpu::registerAccess(Word reg, Byte* pData, bool isWrite)
{
    switch (reg) {
        case 0xff40: BusUtil::simpleRegAccess(&lcdc, pData, isWrite); return;
        // case 0xff41: stat - TODO
        case 0xff42: BusUtil::simpleRegAccess(&scy, pData, isWrite); return;
        case 0xff43: BusUtil::simpleRegAccess(&scx, pData, isWrite); return;
        case 0xff44: {
            // XXX: what happens on LY write?
            if (isWrite)
                log->warn("GPU register write to LY");
            else
                *pData = ly;
            return;
        }
        case 0xff45: BusUtil::simpleRegAccess(&lyc, pData, isWrite); return;
        // case 0xff46: dma - TODO: must be handled by Bus!
        case 0xff47: BusUtil::simpleRegAccess(&bgp, pData, isWrite); return;
        case 0xff4a: BusUtil::simpleRegAccess(&wy, pData, isWrite); return;
        case 0xff4b: BusUtil::simpleRegAccess(&wx, pData, isWrite); return;
    }
    log->warn("Unhandled GPU register %s to register %04X", isWrite ? "write" : "read", reg);
}
