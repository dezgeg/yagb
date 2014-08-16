#pragma once

#include "Cpu.hpp"
#include "Gpu.hpp"
#include "Logger.hpp"
#include "Rom.hpp"

class Gameboy
{
    Logger *log;
    Bus bus;
    Gpu gpu;
    Cpu cpu;
    long currentCycle;

public:
    Gameboy(Logger* log, Rom* rom) :
        log(log),
        bus(log, rom, &gpu),
        gpu(log),
        cpu(log, &bus),
        currentCycle(0)
    {
    }

    Gpu* getGpu() { return &gpu; }
    Bus* getBus() { return &bus; }

    void runFrame();
};
