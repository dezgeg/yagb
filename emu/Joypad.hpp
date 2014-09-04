#pragma once

#include "Platform.hpp"

enum PadKeys {
    Pad_Right = 1 << 0,
    Pad_Left = 1 << 1,
    Pad_Up = 1 << 2,
    Pad_Down = 1 << 3,
    Pad_A = 1 << 4,
    Pad_B = 1 << 5,
    Pad_Select = 1 << 6,
    Pad_Start = 1 << 7,

    Pad_AllKeys = 0xff,
};

class Joypad {
    Byte currentKeys;   // state of keys pressed in GUI, 1 = pressed
    Byte latestKeys;    // latest keys delivered to emulation core
    Byte latches;

    Byte getStateFor(Byte keys) {
        Byte v = 0x0f;
        if (!(latches & 0x1)) {
            v &= ~(keys & 0x0f);
        }
        if (!(latches & 0x2)) {
            v &= ~(keys >> 4);
        }
        return v;
    }

public:
    Joypad() :
            currentKeys(0),
            latestKeys(0),
            latches(0) {
    }

    bool tick() {
        Byte oldSt = getStateFor(latestKeys);
        Byte newSt = getStateFor(currentKeys);
        bool ret = (oldSt ^ newSt) & oldSt;

        latestKeys = currentKeys;
        return ret;
    }

    void regAccess(Byte* pData, bool isWrite) {
        if (isWrite) {
            latches = (*pData >> 4) & 0x3;
        } else {
            *pData = getStateFor(latestKeys) | (latches << 4);
        }
    }

    void keysPressed(Byte keys) {
        currentKeys |= keys;
    }

    void keysReleased(Byte keys) {
        currentKeys &= ~keys;
    }
};
