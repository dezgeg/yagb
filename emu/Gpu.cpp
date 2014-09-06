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

    MaxScanline = 153,
};

IrqSet Gpu::tick(long cycles) {
    IrqSet irqs = 0;

    bool wasInOamFetch = cycleResidue < OamFetchThresholdCycles;
    bool wasInHBlank = cycleResidue >= VramFetchThresholdCycles;

    cycleResidue += cycles;

    bool nowInOamFetch = cycleResidue < OamFetchThresholdCycles;
    bool nowInHBlank = cycleResidue >= VramFetchThresholdCycles;

    if (cycleResidue >= ScanlineCycles) {
        // Assumes that cycle delta is not insanely large
        cycleResidue -= ScanlineCycles;
        regs.ly++;
        if (regs.ly > MaxScanline) {
            regs.ly = 0;
            frame++;
        } else if (regs.ly == ScreenHeight) {
            irqs |= bit(Irq_VBlank);
            if (regs.vBlankIrqEnabled) {
                irqs |= bit(Irq_LcdStat);
            }
        }

        if (regs.ly == regs.lyc && regs.coincidenceIrqEnabled) {
            irqs |= bit(Irq_LcdStat);
        }
    } else {
        if (!nowInOamFetch && wasInOamFetch) { // TODO - bug
            captureSpriteState();
            if (regs.oamIrqEnabled) {
                irqs |= bit(Irq_LcdStat);
            }
        }
        if (nowInHBlank && !wasInHBlank) {
            if (regs.ly < ScreenHeight) {
                renderScanline();
            }

            if (regs.hBlankIrqEnabled) {
                irqs |= bit(Irq_LcdStat);
            }
        }
    }

    return irqs;
}

void Gpu::captureSpriteState() {
    int prevSpriteXPos = -1000;
    for (unsigned i = 0; i < 10; i++) {
        int bestSpriteNum = -1;
        int bestXPos = 1000;

        for (unsigned j = 0; j < 40; j++) {
            int spriteTop = sprites[j].y - 16;
            int spriteBottom = spriteTop + (regs.objSizeLarge ? 16 : 8);

            if (!(regs.ly >= spriteTop && regs.ly < spriteBottom)) {
                continue;
            } // not visible this scanline
            if (sprites[j].x > prevSpriteXPos && sprites[j].x < bestXPos) {
                bestSpriteNum = j;
                bestXPos = sprites[j].x;
            }
        }
        visibleSprites[i] = bestSpriteNum;
        prevSpriteXPos = bestXPos;
    }
}

void Gpu::renderScanline() {
    Byte* bgPatternBase = regs.bgPatternBaseSelect ? &vram[0x0] : &vram[0x1000];  // Bit 4

    unsigned spriteIndex = 0;
    for (unsigned i = 0; i < ScreenWidth; i++) {
        if (!regs.lcdEnabled || !regs.bgEnabled) {
            framebuffer[regs.ly][i] = 0;
            continue;
        }

        int scrollX, scrollY;
        bool bgOrWinEnabled;
        bool tileBaseSelector;
        if (regs.winEnabled && regs.ly >= regs.wy && (int)i >= regs.wx - 7) {
            scrollX = -regs.wx + 7;
            scrollY = -regs.wy;
            bgOrWinEnabled = true;
            tileBaseSelector = regs.winTileBaseSelect; // Bit 6
        } else {
            scrollX = regs.scx;
            scrollY = regs.scy;
            bgOrWinEnabled = regs.bgEnabled;
            tileBaseSelector = regs.bgTileBaseSelect; // Bit 3
        }
        Byte* bgTileBase = tileBaseSelector ? &vram[0x1c00] : &vram[0x1800];    // Bit 3/6
        unsigned bgY = regs.ly + scrollY;
        unsigned bgTileY = (bgY / 8) % 32;
        unsigned bgTileYBit = bgY % 8;

        Byte bgColor = 0;
        Byte pixel = 0;
        if (bgOrWinEnabled) {
            unsigned bgX = i + scrollX;
            unsigned bgTileX = (bgX / 8) % 32;
            unsigned bgTileXBit = bgX % 8;

            Byte tileNum = bgTileBase[bgTileY * 32 + bgTileX];
            long tileOff = regs.bgPatternBaseSelect ? (long)tileNum : (long)(SByte)tileNum;
            bgColor = drawTilePixel(bgPatternBase + 16 * tileOff, bgTileXBit, bgTileYBit);
            pixel = applyPalette(regs.bgp, bgColor);
        }

        trySpriteAgain:
        if (regs.objEnabled && spriteIndex < 10 && visibleSprites[spriteIndex] >= 0) {
            OamEntry* oamEntry = &sprites[visibleSprites[spriteIndex]];

            int tileX = i - (oamEntry->x - 8);
            if (tileX < 0) {
                goto noSprite;
            }
            assert(tileX <= 8);
            if (tileX == 8) {
                spriteIndex++;
                goto trySpriteAgain;
            }
            int tileY = regs.ly - (oamEntry->y - 16);
            assert(tileY >= 0 && tileX < 16);
            Byte palette = oamEntry->flags.palette ? regs.obp1 : regs.obp0;

            Byte spriteColor = drawTilePixel(&vram[16 * oamEntry->tile], tileX, tileY,
                    regs.objSizeLarge, oamEntry->flags);
            Byte spritePixel = applyPalette(palette, spriteColor);
            if (spriteColor != 0 && (!oamEntry->flags.lowPriority || bgColor == 0)) {
                pixel = spritePixel;
            }
        }
        noSprite:

        framebuffer[regs.ly][i] = pixel;
    }
}

void Gpu::vramAccess(Word offset, Byte* pData, bool isWrite) {
#if 0
    if (isWrite)
        log->warn("GPU VRAM write [0x%0x] = 0x%02x", 0x8000 + offset, *pData);
#endif
    BusUtil::arrayMemAccess(vram, offset, pData, isWrite);
}

void Gpu::oamAccess(Word offset, Byte* pData, bool isWrite) {
#if 0
    if (isWrite)
        log->warn("GPU OAM write [0x%0x] = 0x%02x", 0xfe00 + offset, *pData);
#endif
    BusUtil::arrayMemAccess(oam, offset, pData, isWrite);
}

void Gpu::registerAccess(Word reg, Byte* pData, bool isWrite) {
#if 0
    if (isWrite)
        log->warn("GPU reg write [0x%0x] = 0x%02x", reg, *pData);
#endif

    switch (reg) {
        case 0xff40:
            BusUtil::simpleRegAccess(&regs.lcdc, pData, isWrite);
            return;
        case 0xff41: {
            if (isWrite) {
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
        case 0xff42:
            BusUtil::simpleRegAccess(&regs.scy, pData, isWrite);
            return;
        case 0xff43:
            BusUtil::simpleRegAccess(&regs.scx, pData, isWrite);
            return;
        case 0xff44: {
            // XXX: what happens on LY write?
            if (isWrite) {
                log->warn("GPU register write to LY");
            } else {
                *pData = regs.ly;
            }
            return;
        }
        case 0xff45:
            BusUtil::simpleRegAccess(&regs.lyc, pData, isWrite);
            return;
            // case 0xff46: dma - TODO: must be handled by Bus!
        case 0xff47:
            BusUtil::simpleRegAccess(&regs.bgp, pData, isWrite);
            return;
        case 0xff48:
            BusUtil::simpleRegAccess(&regs.obp0, pData, isWrite);
            return;
        case 0xff49:
            BusUtil::simpleRegAccess(&regs.obp1, pData, isWrite);
            return;
        case 0xff4a:
            BusUtil::simpleRegAccess(&regs.wy, pData, isWrite);
            return;
        case 0xff4b:
            BusUtil::simpleRegAccess(&regs.wx, pData, isWrite);
            return;
    }
    log->warn("Unhandled GPU register %s to register %04X", isWrite ? "write" : "read", reg);
}
