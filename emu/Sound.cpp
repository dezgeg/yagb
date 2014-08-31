#include "Sound.hpp"

void Sound::registerAccess(Word address, Byte* pData, bool isWrite)
{
    if (address == 0xff15 || address == 0xff1f ||
            (address >= 0xff27 && address <= 0xff2f)) {
        log->warn("Unimplemented sound reg 0x%04x", address);
        return;
    }

    // TODO: mask
    BusUtil::arrayMemAccess((Byte*)&regs, address - 0xff10, pData, isWrite);
}
