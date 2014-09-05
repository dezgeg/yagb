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
        log(log) {
    std::ifstream stream(fileName, std::ios_base::in | std::ios_base::binary);
    stream.seekg(0, std::ios::end);
    if (!stream) {
        throw "No such file";
    }

    size_t sz = stream.tellg();
    romData.resize(sz);
    stream.seekg(0, std::ios::beg);
    stream.read(&romData[0], sz);

    std::memset(ramData, 0, sizeof(ramData));

    std::memset(&mapperRegs, 0, sizeof(mapperRegs));
    mapperRegs.romBankLowBits = 1;

    Byte mapperByte = romData[MapperOffset];
    if (mapperByte == 0x00) {
        mapper = Mapper_None;
    } else if (mapperByte >= 0x01 && mapperByte <= 0x03) {
        mapper = Mapper_MBC1;
    } else if (mapperByte >= 0x0f && mapperByte <= 0x13) {
        mapper = Mapper_MBC3;
    } else
        assert(!"Unknown mapper");
}

void Rom::cartRomAccess(Word address, Byte* pData, bool isWrite) {
    if (isWrite) {
        if (!mapper) {
            log->warn("Write (0x%02x) to ROM address 0x%04x without mapper", *pData, address);
            return;
        }
        // log->warn("Mapper write %04x (%d)", address, address >> 13);
        switch (address >> 13) {
            case 0: // 0000-1FFF
                mapperRegs.ramEnabled = (*pData & 0x0f) == 0x0a;
                break;
            case 1: // 2000-3FFF
                if (mapper == Mapper_MBC1) {
                    mapperRegs.romBankLowBits = *pData & 0x1f;
                } else if (mapper == Mapper_MBC3) {
                    mapperRegs.romBankLowBits = *pData & 0x7f;
                } else
                    assert(0);

                if (!mapperRegs.romBankLowBits) {
                    mapperRegs.romBankLowBits = 1;
                }
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
            unsigned bank;
            if (mapper == Mapper_MBC1) {
                unsigned highBits = mapperRegs.bankingMode ? 0 : mapperRegs.bankHighBits;
                bank = (highBits << 5) | mapperRegs.romBankLowBits;
            } else if (mapper == Mapper_MBC3) {
                bank = mapperRegs.romBankLowBits;
            } else {
                assert(0);
            }
            assert(bank);
            *pData = romData[bank * 0x4000 + address - 0x4000];
        }
    }
}

void Rom::cartRamAccess(Word address, Byte* pData, bool isWrite) {
#if 0
    if (!mapper) {
        log->warn("Access to cart RAM (0x%04x) without mapper", address);
    }
#endif
    BusUtil::arrayMemAccess(ramData, address, pData, isWrite);
}
