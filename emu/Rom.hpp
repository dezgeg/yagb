#pragma once
#include "Platform.hpp"
#include "Logger.hpp"

#include <string>

enum Mapper {
    Mapper_None,
    Mapper_MBC1,
    Mapper_MBC3,
};

class Rom
{
    Logger* log;
    std::string romData;
    Byte ramData[8192];
    Mapper mapper;

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
