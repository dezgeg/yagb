#pragma once

#include "Cpu.hpp"
#include "Gpu.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Timer.hpp"

class Gameboy
{
    Logger *log;
    Bus bus;
    Gpu gpu;
    Cpu cpu;
    Timer timer;
    long currentCycle;

public:
    Gameboy(Logger* log, Rom* rom) :
        log(log),
        bus(log, rom, &gpu, &timer),
        gpu(log),
        cpu(log, &bus),
        timer(),
        currentCycle(0)
    {
    }

    Bus* getBus() { return &bus; }
    Cpu* getCpu() { return &cpu; }
    Gpu* getGpu() { return &gpu; }
    Timer* getTimer() { return &timer; }

    void runFrame();
};
