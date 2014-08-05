#include "Rom.hpp"
#include "Gameboy.hpp"

Rom::Rom(Gameboy* gb, const char* fileName) :
    gb(gb)
{
    std::ifstream stream(fileName, std::ios_base::in | std::ios_base::binary);
    stream.seekg(0, std::ios::end);
    if (!stream)
        throw "No such file";

    size_t sz = stream.tellg();
    buf.resize(sz);
    stream.seekg(0, std::ios::beg);
    stream.read(&buf[0], sz);
}

void Rom::memAccess(Word address, Byte* pData, bool isWrite)
{
    if (isWrite) {
        gb->warn("Write to ROM");
        return;
    }

    *pData = address < buf.size() ? buf[address] : 0;
}
