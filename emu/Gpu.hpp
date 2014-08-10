#pragma once
#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

class Gpu
{
    Logger* log;
    Byte vram[8192];

public:
    Gpu(Logger* log) :
        log(log)
    {
    }

    void vramAccess(Word offset, Byte* pData, bool isWrite);
    void registerAccess(Word reg, Byte* pData, bool isWrite);

    void tick(long cycles);
};
