#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Utils.hpp"

#include <fstream>
#include <cstring>

enum {
    MapperOffset = 0x0147,
};

Rom::Rom(Logger* log, const char* fileName) :
    log(log)
{
    std::ifstream stream(fileName, std::ios_base::in | std::ios_base::binary);
    stream.seekg(0, std::ios::end);
    if (!stream)
        throw "No such file";

    size_t sz = stream.tellg();
    romData.resize(sz);
    stream.seekg(0, std::ios::beg);
    stream.read(&romData[0], sz);

    mapper = romData[MapperOffset];
    std::memset(ramData, 0, sizeof(ramData));

    std::memset(&mapperRegs, 0, sizeof(mapperRegs));
    mapperRegs.romBankLowBits = 1;
}

void Rom::cartRomAccess(Word address, Byte* pData, bool isWrite)
{
    if (isWrite) {
        if (!mapper) {
            log->warn("Write (0x%02x) to ROM address 0x%04x without mapper", *pData, address);
            return;
        }
        log->warn("Mapper write %04x (%d)", address, address >> 13);
        switch (address >> 13) {
            case 0: // 0000-1FFF
                mapperRegs.ramEnabled = (*pData & 0x0f) == 0x0a;
                break;
            case 1: // 2000-3FFF
                mapperRegs.romBankLowBits = *pData & 0x1f;
                if (!mapperRegs.romBankLowBits)
                    mapperRegs.romBankLowBits = 1;
                break;
            case 2: // 4000-5FFF
                mapperRegs.bankHighBits = *pData & 0x03;
                break;
            case 3: // 6000-7FFF
                mapperRegs.bankingMode = *pData & 0x01;
                break;
        }
    } else {
        if (!mapper || address < 0x4000) {
            *pData = address < romData.size() ? romData[address] : 0;
        } else {
            unsigned highBits = mapperRegs.bankingMode ? 0 : mapperRegs.bankHighBits;
            unsigned bank = (highBits << 5) | mapperRegs.romBankLowBits;
            assert(bank);
            *pData = romData[bank * 0x4000 + address - 0x4000];
        }
    }
}

void Rom::cartRamAccess(Word address, Byte* pData, bool isWrite)
{
    if (!mapper)
        log->warn("Access to cart RAM (0x%04x) without mapper", address);
    BusUtil::arrayMemAccess(ramData, address, pData, isWrite);
}
