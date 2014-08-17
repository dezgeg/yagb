#pragma once
#include "Platform.hpp"
#include "Logger.hpp"

#include <string>

class Rom
{
    Logger* log;
    std::string romData;
    Byte ramData[8192];
    Byte mapper;

    struct MapperRegs {
        bool ramEnabled;
        bool bankingMode;
        Byte romBankLowBits;
        Byte bankHighBits;      // if bankingMode == 1, selects RAM bank, else selects ROM bank
    } mapperRegs;

public:
    Rom(Logger* log, const char* fileName);

    void cartRomAccess(Word address, Byte* pData, bool isWrite);
    void cartRamAccess(Word address, Byte* pData, bool isWrite);
};
