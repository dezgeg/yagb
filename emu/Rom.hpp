#pragma once
#include "Platform.hpp"
#include "Logger.hpp"

#include <string>

class Gameboy;
class Rom
{
    Logger* log;
    std::string buf;

public:
    Rom(Logger* log, const char* fileName);

    void memAccess(Word address, Byte* pData, bool isWrite);
};
