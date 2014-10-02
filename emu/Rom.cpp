#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Utils.hpp"
#include "Serializer.hpp"

#include <fstream>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static constexpr Word MapperOffset = 0x0147;

static constexpr size_t MAX_SAVE_RAM_SIZE = 0x10000;

Rom::Rom(Logger* log, const char* fileName) :
        log(log),
        saveRamFd(-1),
        saveRamData((Byte*)MAP_FAILED) {

    readRomFile(fileName);
    setupSaveRam(fileName);
    setupMapper();
}

void Rom::readRomFile(char const* fileName) {
    std::ifstream stream(fileName, std::ios_base::in | std::ios_base::binary);
    stream.seekg(0, std::ios_base::end);
    if (!stream) {
        throw "No such file";
    }

    size_t sz = stream.tellg();
    romData.resize(sz);
    stream.seekg(0, std::ios_base::beg);
    stream.read((char*)&romData[0], sz);
}

void Rom::setupSaveRam(char const* name) {
    std::string saveRamFile = replaceExtension(name, "sav");

    saveRamFd = open(saveRamFile.c_str(), O_CREAT | O_RDWR, 0644);
    if (saveRamFd < 0) {
        throw "Can't open save RAM file";
    }
    if (ftruncate(saveRamFd, MAX_SAVE_RAM_SIZE) < 0) {
        throw "Can't resize save RAM file";
    }
    saveRamData = (Byte*)mmap(nullptr, MAX_SAVE_RAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, saveRamFd, 0);
    if (saveRamData == MAP_FAILED) {
        throw "Can't mmap save RAM file";
    }
}

void Rom::setupMapper() {
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
                mapperRegs.rtcRegsEnabled = *pData & 0x08;
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
    if (mapper && !mapperRegs.ramEnabled) {
        log->warn("Access to cart RAM without enabling it");
        return;
    }

    unsigned bank = 0;
    if (mapper == Mapper_MBC1) {
        bank = mapperRegs.bankingMode ? mapperRegs.bankHighBits : 0;
    } else if (mapper == Mapper_MBC3) {
        if (mapperRegs.rtcRegsEnabled) {
            log->warn("RTC not implemented!");
            return;
        }
        bank = mapperRegs.bankHighBits;
    }
    BusUtil::arrayMemAccess(saveRamData, address + bank * 0x4000, pData, isWrite);
}

void Rom::serialize(Serializer& ser) {
    ser.handleObject("Rom.mapper", mapper);
    ser.handleObject("Rom.mapperRegs", mapperRegs);
    ser.handleByteBuffer("Rom.saveRamData", saveRamData, MAX_SAVE_RAM_SIZE);
}

Rom::~Rom() {
    if (saveRamData != MAP_FAILED) {
        munmap(saveRamData, MAX_SAVE_RAM_SIZE);
    }
    if (saveRamFd != -1) {
        close(saveRamFd);
    }
}
