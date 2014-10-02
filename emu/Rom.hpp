#pragma once

#include "Platform.hpp"
#include "Logger.hpp"
#include "Serializer.hpp"

#include <vector>

enum Mapper {
    Mapper_None,
    Mapper_MBC1,
    Mapper_MBC3,
};

class Rom {
    Logger* log;
    const char* fileName;
    std::vector<Byte> romData;

    int saveRamFd;
    Byte* saveRamData;

    Mapper mapper;

    struct MapperRegs {
        bool ramEnabled;
        bool rtcRegsEnabled;
        bool bankingMode;
        Byte romBankLowBits;
        Byte bankHighBits;      // if bankingMode == 1, selects RAM bank, else selects ROM bank
    } mapperRegs;

    void readRomFile(const char* fileName);
    void setupSaveRam(const char* name);
    void setupMapper();

public:
    Rom(Logger* log, const char* fileName);
    ~Rom();

    void cartRomAccess(Word address, Byte* pData, bool isWrite);
    void cartRamAccess(Word address, Byte* pData, bool isWrite);
    void serialize(Serializer& ser);

    const char* getFileName() { return fileName; }
};
