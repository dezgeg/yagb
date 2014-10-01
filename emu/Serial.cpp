#include "BusUtil.hpp"
#include "Serial.hpp"
#include "Serializer.hpp"

bool Serial::tick(int cycles) {
    if (!isRunning()) {
        return false;
    }

    currentCycles += cycles;
    // 8192 Hz => Divisor of 2^1, 8 bits of data
    if (currentCycles >= 8 * (1 << 11)) {
        regs.sb = 0xFF; // No other gameboy connected
        regs.transferStart = false;
        return true;
    }

    return false;
}

void Serial::regAccess(Word address, Byte* pData, bool isWrite) {
    if (address == 0) {
        BusUtil::simpleRegAccess(&regs.sb, pData, isWrite);
    } else {
        bool wasRunning = isRunning();
        BusUtil::simpleRegAccess(&regs.sc, pData, isWrite, 0x83);
        if (!wasRunning && isRunning()) {
            currentCycles = 0;
        }
    }
}
void Serial::serialize(Serializer& ser) {
    ser.handleObject("Serial.currentCycles", currentCycles);
    ser.handleObject("Serial.regs", regs);
}
