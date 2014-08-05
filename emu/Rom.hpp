#pragma once
#include "Platform.hpp"

#include <string>
#include <fstream>

class Gameboy;
class Rom
{
    Gameboy* gb;
    std::string buf;

public:
    Rom(Gameboy* gb, const char* fileName);

    void memAccess(Word address, Byte* pData, bool isWrite);
};
