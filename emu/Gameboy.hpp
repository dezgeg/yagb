#pragma once

#include "Cpu.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Serial.hpp"
#include "Timer.hpp"

class Gameboy
{
    Logger *log;
    Bus bus;
    Gpu gpu;
    Cpu cpu;
    Timer timer;
    Joypad joypad;
    Serial serial;
    long currentCycle;

public:
    Gameboy(Logger* log, Rom* rom) :
        log(log),
        bus(log, rom, &gpu, &timer, &joypad, &serial),
        gpu(log),
        cpu(log, &bus),
        timer(),
        joypad(),
        serial(),
        currentCycle(0)
    {
    }

    Bus* getBus() { return &bus; }
    Cpu* getCpu() { return &cpu; }
    Gpu* getGpu() { return &gpu; }
    Timer* getTimer() { return &timer; }
    Joypad* getJoypad() { return &joypad; }

    void runFrame();
};
