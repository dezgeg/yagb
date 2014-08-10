#pragma once

#include "Cpu.hpp"
#include "Logger.hpp"
#include "Rom.hpp"

class Gameboy
{
    Logger *log;
    Bus bus;
    Cpu cpu;
    long currentCycle;

public:
    Gameboy(Logger* log, Rom* rom) :
        log(log),
        bus(log, rom),
        cpu(log, &bus),
        currentCycle(0)
    {
    }

    void run();
};
