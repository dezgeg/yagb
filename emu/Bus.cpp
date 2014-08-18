#include "Bus.hpp"
#include "BusUtil.hpp"
#include "Gpu.hpp"
#include "Irq.hpp"
#include "Joypad.hpp"
#include "Rom.hpp"
#include "Timer.hpp"
#include "Utils.hpp"

#include <algorithm>

static const Byte bootrom[] =
    "\x31\xfe\xff\xaf\x21\xff\x9f\x32\xcb\x7c\x20\xfb\x21\x26\xff\x0e"
    "\x11\x3e\x80\x32\xe2\x0c\x3e\xf3\xe2\x32\x3e\x77\x77\x3e\xfc\xe0"
    "\x47\x11\x04\x01\x21\x10\x80\x1a\xcd\x95\x00\xcd\x96\x00\x13\x7b"
    "\xfe\x34\x20\xf3\x11\xd8\x00\x06\x08\x1a\x13\x22\x23\x05\x20\xf9"
    "\x3e\x19\xea\x10\x99\x21\x2f\x99\x0e\x0c\x3d\x28\x08\x32\x0d\x20"
    "\xf9\x2e\x0f\x18\xf3\x67\x3e\x64\x57\xe0\x42\x3e\x91\xe0\x40\x04"
    "\x1e\x02\x0e\x0c\xf0\x44\xfe\x90\x20\xfa\x0d\x20\xf7\x1d\x20\xf2"
    "\x0e\x13\x24\x7c\x1e\x83\xfe\x62\x28\x06\x1e\xc1\xfe\x64\x20\x06"
    "\x7b\xe2\x0c\x3e\x87\xe2\xf0\x42\x90\xe0\x42\x15\x20\xd2\x05\x20"
    "\x4f\x16\x20\x18\xcb\x4f\x06\x04\xc5\xcb\x11\x17\xc1\xcb\x11\x17"
    "\x05\x20\xf5\x22\x23\x22\x23\xc9\xce\xed\x66\x66\xcc\x0d\x00\x0b"
    "\x03\x73\x00\x83\x00\x0c\x00\x0d\x00\x08\x11\x1f\x88\x89\x00\x0e"
    "\xdc\xcc\x6e\xe6\xdd\xdd\xd9\x99\xbb\xbb\x67\x63\x6e\x0e\xec\xcc"
    "\xdd\xdc\x99\x9f\xbb\xb9\x33\x3e\x3c\x42\xb9\xa5\xb9\xa5\x42\x3c"
    "\x21\x04\x01\x11\xa8\x00\x1a\x13\xbe\x20\xfe\x23\x7d\xfe\x34\x20"
    "\xf5\x06\x19\x78\x86\x23\x05\x20\xfb\x86\x20\xfe\x3e\x01\xe0\x50";

void Bus::tickDma(int cycles)
{
    if (!dmaInProgress)
        return;

    assert(cycles % 4 == 0);
    const int maxCycles = 4 * 4 * 40; // XXX: does it really take 4 cycles for each byte?
    int nextCycles = std::min(dmaCycles + cycles, maxCycles);

    for (int i = dmaCycles; i < nextCycles; i++) {
        Byte data = 0;
        memAccess((dmaSourcePage << 8) | (i / 4), &data, false, false);
        gpu->oamAccess(i / 4, &data, true);
    }
    if (nextCycles == maxCycles)
        dmaInProgress = false;
}

void Bus::dmaRegAccess(Byte* pData, bool isWrite)
{
    if (isWrite) {
        dmaInProgress = true;
        dmaCycles = 0;
        dmaSourcePage = *pData;
    } else {
        *pData = dmaSourcePage;
    }
}

void Bus::disableBootrom()
{
    bootromEnabled = false;
}

void Bus::memAccess(Word address, Byte* pData, bool isWrite, bool emulatorInternal)
{
    if (address <= 0xff) {
        if (bootromEnabled) {
            if (isWrite)
                log->warn("Write to BootRom");
            else
                BusUtil::arrayMemAccess(const_cast<Byte*>(bootrom), address, pData, false);
        } else {
            rom->cartRomAccess(address, pData, isWrite);
        }
    } else if (address <= 0x7fff)
        rom->cartRomAccess(address, pData, isWrite);
    else if (address <= 0x9fff)
        gpu->vramAccess(address & 0x1fff, pData, isWrite);
    else if (address <= 0xbfff)
        rom->cartRamAccess(address & 0x1fff, pData, isWrite);
    else if (address <= 0xfdff)
        BusUtil::arrayMemAccess(ram, address & 0x1fff, pData, isWrite);
    else if (address <= 0xfe9f)
        gpu->oamAccess(address & 0xff, pData, isWrite);
    else if (address == 0xff00)
        joypad->regAccess(pData, isWrite); // log->warn("JoyPad: %s %02x", isWrite ? "WR" : "RD", *pData);
    else if (address >= 0xff04 && address <= 0xff07)
        timer->regAccess(address, pData, isWrite);
    else if (address == 0xff0f)
        BusUtil::simpleRegAccess(&irqsPending, pData, isWrite, 0x1f);
    else if (address == 0xff46)
        dmaRegAccess(pData, isWrite);
    else if (address >= 0xff40 && address <= 0xff4b)
        gpu->registerAccess(address, pData, isWrite);
    else if (address == 0xff50)
        disableBootrom();
    else if (address >= 0xff80 && address <= 0xfffe)
        BusUtil::arrayMemAccess(hram, address - 0xff80, pData, isWrite);
    else if (address == 0xffff)
        BusUtil::simpleRegAccess(&irqsEnabled, pData, isWrite, 0x1f);
    else {
        if (isWrite)
            log->warn("Unhandled write (0x%02x) to address 0x%04X", *pData, address);
        else
            log->warn("Unhandled read from address 0x%04X", address);
    }

    if (!bootromEnabled && !emulatorInternal)
        log->logMemoryAccess(address, *pData, isWrite);
}

Byte Bus::memRead8(Word address, bool emulatorInternal)
{
    Byte value = 0;
    memAccess(address, &value, false, emulatorInternal);
    return value;
}

void Bus::memWrite8(Word address, Byte value, bool emulatorInternal)
{
    memAccess(address, &value, true, emulatorInternal);
}

Word Bus::memRead16(Word address, bool emulatorInternal)
{
    return memRead8(address, emulatorInternal) | (memRead8(address + 1, emulatorInternal) << 8);
}

void Bus::memWrite16(Word address, Word value, bool emulatorInternal)
{
    memWrite8(address, (Byte)(value), emulatorInternal);
    memWrite8(address + 1, (Byte)(value >> 8), emulatorInternal);
}

void Bus::raiseIrq(IrqSet irqs)
{
    irqsPending |= irqs & irqsEnabled;
}

void Bus::ackIrq(Irq irq)
{
    if (!(irqsPending & (1 << irq)))
        log->warn("IRQ %d not pending?", irq);
    irqsPending &= ~(1 << irq);
}

IrqSet Bus::getEnabledIrqs()
{
    return irqsEnabled;
}

IrqSet Bus::getPendingIrqs()
{
    return irqsPending & irqsEnabled;
}

bool Bus::isBootromEnabled()
{
    return bootromEnabled;
}
