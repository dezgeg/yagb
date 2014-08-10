#include "Logger.hpp"
#include "Rom.hpp"

#include <fstream>

Rom::Rom(Logger* log, const char* fileName) :
    log(log)
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
        log->warn("Write to ROM");
        return;
    }

    *pData = address < buf.size() ? buf[address] : 0;
}
