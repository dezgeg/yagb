#pragma once

#include "Platform.hpp"
#include "Serializer.hpp"

// Doesn't actually communicate with anything, but
// some games (Alleyway) use the serial for timing. UGH!
class Serial {
    long currentCycles;
    struct Regs {
        Byte sb;
        union {
            Byte sc;
            struct {
                Byte internalClock : 1;
                Byte unused : 6;
                Byte transferStart : 1;
            };
        };
    } regs;

    inline bool isRunning() {
        return regs.transferStart && regs.internalClock;
    }

public:
    Serial() :
            currentCycles(0),
            regs() {
    }

    bool tick(int cycles);
    void regAccess(Word offset, Byte* pData, bool isWrite);
    void serialize(Serializer& ser);
};
