#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Rom.hpp"

#include <fstream>
#include <cstring>

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

    std::memset(ramData, 0, sizeof(ramData));
}

void Rom::cartRomAccess(Word address, Byte* pData, bool isWrite)
{
    if (isWrite) {
        log->warn("Write (0x%02x) to ROM address 0x%04x", *pData, address);
        return;
    }

    *pData = address < romData.size() ? romData[address] : 0;
}

void Rom::cartRamAccess(Word address, Byte* pData, bool isWrite)
{
    BusUtil::arrayMemAccess(ramData, address, pData, isWrite);
}
