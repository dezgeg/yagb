#pragma once
#include "Platform.hpp"
#include "Logger.hpp"

#include <string>

class Rom
{
    Logger* log;
    std::string romData;
    Byte ramData[8192];

public:
    Rom(Logger* log, const char* fileName);

    void cartRomAccess(Word address, Byte* pData, bool isWrite);
    void cartRamAccess(Word address, Byte* pData, bool isWrite);
};
