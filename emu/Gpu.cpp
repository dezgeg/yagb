#include "Gpu.hpp"
#include "Serializer.hpp"
#include <algorithm>

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
    } else if (regs.ly < ScreenHeight) {
        if (!nowInOamFetch && wasInOamFetch && renderEnabled) {
            captureSpriteState();
        }
        if (nowInOamFetch && !wasInHBlank) {
            if (regs.oamIrqEnabled) {
                irqs |= bit(Irq_LcdStat);
            }
        }
        if (nowInHBlank && !wasInHBlank) {
            if (renderEnabled) {
                renderScanline();
            }

            if (regs.hBlankIrqEnabled) {
                irqs |= bit(Irq_LcdStat);
            }
        }
    }

    // TODO FIXME: instead of just not raising IRQs, force vblank when LCD
    // is off, and restart rendering at LY=0 when display re-enabled
    return regs.lcdEnabled ? irqs : 0;
}

Byte Gpu::drawTilePixel(Byte* tile, unsigned x, unsigned y, bool large, OamEntry::OamFlags flags) {
    Byte height = large ? 16 : 8;
    unsigned base = 2 * (flags.yFlip ? height - y - 1 : y);

    if (bus->isGbcMode() && flags.cgbTileVramBank) {
        // Slight hack...
        tile += 8192;
    }

    Byte lsbs = flags.xFlip ? reverseBits(tile[base + 0]) : tile[base + 0];
    Byte msbs = flags.xFlip ? reverseBits(tile[base + 1]) : tile[base + 1];

    return (!!(lsbs & (0x80 >> x))) |
            ((!!(msbs & (0x80 >> x))) << 1);
}

void Gpu::captureSpriteState() {
    unsigned num = 0;
    for (unsigned j = 0; j < 40; j++) {
        int spriteTop = sprites[j].y - 16;
        int spriteBottom = spriteTop + (regs.objSizeLarge ? 16 : 8);

        if (!(regs.ly >= spriteTop && regs.ly < spriteBottom)) {
            // not visible this scanline
            continue;
        }
        visibleSprites[num++] = j;
    }
    auto callback = [this](SByte i, SByte j) -> bool {
        if (sprites[i].x < sprites[j].x) {
            return true;
        } else if (sprites[i].x == sprites[j].x) {
            return i < j;
        } else {
            return false;
        }
    };
    std::sort(visibleSprites, visibleSprites + num, callback);
    for (int i = num; i < 10; ++i) {
        visibleSprites[i] = -1;
    }
}

void Gpu::renderScanline() {
    Byte* bgPatternBase = regs.bgPatternBaseSelect ? &vram[0x0] : &vram[0x1000];  // Bit 4

    for (unsigned i = 0; i < ScreenWidth; i++) {
        if (!regs.lcdEnabled || !regs.bgEnabled) {
            framebuffer[regs.ly][i] = 0;
            continue;
        }

        int scrollX, scrollY;
        bool bgOrWinEnabled;
        bool tileBaseSelector;
        bool isWindow = false;
        if (regs.winEnabled && regs.ly >= regs.wy && (int)i >= regs.wx - 7) {
            scrollX = -regs.wx + 7;
            scrollY = -regs.wy;
            bgOrWinEnabled = true;
            isWindow = true;
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

            unsigned int tileIndex = bgTileY * 32 + bgTileX;
            Byte tileNum = bgTileBase[tileIndex];
            long tileOff = regs.bgPatternBaseSelect ? (long)tileNum : (long)(SByte)tileNum;
            OamEntry::OamFlags attrs;
            attrs.byteVal = (!isWindow && bus->isGbcMode()) ? (bgTileBase + 8192)[tileIndex] : 0;
            bgColor = drawTilePixel(bgPatternBase + 16 * tileOff, bgTileXBit, bgTileYBit, false, attrs);
            pixel = applyPalette(regs.bgp, bgColor);
        }

        for (int j = 0; regs.objEnabled && j < 10 && visibleSprites[j] >= 0; ++j) {
            OamEntry* oamEntry = &sprites[visibleSprites[j]];

            int tileX = i - (oamEntry->x - 8);
            if (tileX < 0 || tileX >= 8) {
                continue;
            }
            int tileY = regs.ly - (oamEntry->y - 16);
            assert(tileY >= 0 && tileY < 16); // XXX: this assert has fired as well!
            Byte palette = oamEntry->flags.dmgPalette ? regs.obp1 : regs.obp0;

            Byte spriteColor = drawTilePixel(&vram[16 * oamEntry->tile], tileX, tileY,
                    regs.objSizeLarge, oamEntry->flags);
            Byte spritePixel = applyPalette(palette, spriteColor);
            if (spriteColor != 0 && (!oamEntry->flags.lowPriority || bgColor == 0)) {
                pixel = spritePixel;
                break;
            }
        }

        framebuffer[regs.ly][i] = pixel;
    }
}

void Gpu::vramAccess(Word offset, Byte* pData, bool isWrite) {
#if 0
    if (isWrite)
        log->warn("GPU VRAM write [0x%0x] = 0x%02x", 0x8000 + offset, *pData);
#endif
    BusUtil::arrayMemAccess(&vram[bus->isGbcMode() && regs.vramBank ? 8192 : 0], offset, pData, isWrite);
}

void Gpu::oamAccess(Word offset, Byte* pData, bool isWrite) {
#if 0
    if (isWrite)
        log->warn("GPU OAM write [0x%0x] = 0x%02x", 0xfe00 + offset, *pData);
#endif
    BusUtil::arrayMemAccess(oam, offset, pData, isWrite);
}

static void accessPaletteIndexReg(Byte* palette, PaletteIndexReg* paletteReg, Byte* pData, bool isWrite) {
    if (isWrite) {
        palette[paletteReg->index] = *pData;
        if (paletteReg->autoincrement) {
            paletteReg->index++;
        }
    } else {
        *pData = palette[paletteReg->index];
    }
}

void Gpu::registerAccess(Word reg, Byte* pData, bool isWrite) {
#if 0
    if (isWrite)
        log->warn("GPU reg write [0x%0x] = 0x%02x", reg, *pData);
#endif
    if (bus->isGbcMode()) {
        switch (reg) {
            case 0xff4f:
                BusUtil::simpleRegAccess(&regs.vramBank, pData, isWrite, 0x01);
                return;
            case 0xff68:
                BusUtil::simpleRegAccess(&regs.cgbBackgroundPaletteIndex.raw, pData, isWrite, 0xbf);
                return;
            case 0xff69:
                accessPaletteIndexReg(cgbBackgroundPalette, &regs.cgbBackgroundPaletteIndex, pData, isWrite);
                return;
            case 0xff6a:
                BusUtil::simpleRegAccess(&regs.cgbSpritePaletteIndex.raw, pData, isWrite, 0xbf);
                return;
            case 0xff6b:
                accessPaletteIndexReg(cgbSpritePalette, &regs.cgbSpritePaletteIndex, pData, isWrite);
                return;
        }
    }

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

void Gpu::serialize(Serializer& ser) {
    ser.handleObject("Gpu.frame", frame);
    ser.handleObject("Gpu.cycleResidue", cycleResidue);
    ser.handleObject("Gpu.framebuffer", framebuffer);
    ser.handleObject("Gpu.visibleSprites", visibleSprites);
    ser.handleObject("Gpu.regs", regs);
    ser.handleObject("Gpu.vram", vram);
    ser.handleObject("Gpu.oam", oam);
    ser.handleObject("Gpu.cgbBackgroundPalette", cgbBackgroundPalette);
    ser.handleObject("Gpu.cgbSpritePalette", cgbSpritePalette);
}
