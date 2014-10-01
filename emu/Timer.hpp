#pragma once

#include "Platform.hpp"
#include "Serializer.hpp"

class Timer {
    long currentCycles;
    struct Regs {
        Byte div;
        Byte tima;
        Byte tma;
        union {
            Byte tac;
            struct {
                Byte divisorSelect : 2;
                Byte running : 1;
            };
        };
    } regs;

public:
    Timer() :
            currentCycles(0),
            regs() {
    }

    bool tick(int cycles);
    void regAccess(Word offset, Byte* pData, bool isWrite);
    void serialize(Serializer& ser);
};
