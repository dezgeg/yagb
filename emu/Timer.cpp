#include "BusUtil.hpp"
#include "Timer.hpp"
#include "Utils.hpp"

// Frequency-to-divisor mapping:
// 4096   Hz => 1024 (2^10)
// 16384  Hz => 256  (2^8)
// 65536  Hz => 64   (2^6)
// 262144 Hz => 16   (2^4)

static const unsigned divisorShifts[] = { 10, 4, 6, 8, };

bool Timer::tick(int cycles)
{
    long before = currentCycles >> divisorShifts[regs.divisorSelect];
    currentCycles += cycles;
    regs.div = currentCycles >> 8;

    if (!regs.running)
        return false;

    long after = currentCycles >> divisorShifts[regs.divisorSelect];
    long delta = after - before;

    bool overflow = false;
    while (delta--) {
        if (regs.tima == 0xff) {
            overflow = true;
            regs.tima = regs.tma;
        } else {
            regs.tima++;
        }
    }

    return overflow;
}

void Timer::regAccess(Word offset, Byte* pData, bool isWrite)
{
    switch (offset) {
        case 0xff04: {
            if (isWrite)
                regs.div = 0;
            else
                *pData = regs.div;
            return;
        }
        case 0xff05: BusUtil::simpleRegAccess(&regs.tima, pData, isWrite); return;
        case 0xff06: BusUtil::simpleRegAccess(&regs.tma, pData, isWrite); return;
        case 0xff07: BusUtil::simpleRegAccess(&regs.tac, pData, isWrite, 0x07); return;
    }
    unreachable();
}
