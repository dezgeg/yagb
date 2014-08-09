#pragma once

#include "Cpu.hpp"
#include "Logger.hpp"
#include "Rom.hpp"

class Gameboy
{
    Bus bus;
    Cpu cpu;

public:
    Gameboy(Logger* log, Rom* rom) :
        bus(log, rom),
        cpu(log, &bus)
    {
    }

    void run();
};
