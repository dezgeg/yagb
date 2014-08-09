#include "Bus.hpp"

void Bus::memAccess(Word address, Byte* pData, bool isWrite)
{
    if (address <= 0x7fff) {
        return rom->memAccess(address, pData, isWrite);
    } else if (address >= 0xc000 && address <= 0xdfff) {
        if (isWrite)
            ram[address - 0xc000] = *pData;
        else
            *pData = ram[address - 0xc000];
    } else if (address >= 0xff80 && address <= 0xfffe) {
        if (isWrite)
            hram[address - 0xff80] = *pData;
        else
            *pData = hram[address - 0xff80];
    } else {
        log->warn("Unhandled %s to address %04X", isWrite ? "write" : "read", address);
    }
}

Byte Bus::memRead8(Word address)
{
    Byte value = 0;
    memAccess(address, &value, false);
    return value;
}

void Bus::memWrite8(Word address, Byte value)
{
    memAccess(address, &value, true);
}

Word Bus::memRead16(Word address)
{
    return memRead8(address) | (memRead8(address + 1) << 8);
}

void Bus::memWrite16(Word address, Word value)
{
    memWrite8(address, (Byte)(value));
    memWrite8(address + 1, (Byte)(value >> 8));
}
