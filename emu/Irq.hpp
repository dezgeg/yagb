#pragma once
#include "Platform.hpp"
#include "Utils.hpp"

enum Irq
{
    Irq_None = -1,

    Irq_VBlank,
    Irq_LcdStat,
    Irq_Timer,
    Irq_Serial,
    Irq_Joystick,

    Irq_Max,
};

typedef Byte IrqSet;
