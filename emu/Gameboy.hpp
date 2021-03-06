#pragma once

#include "Cpu.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Serial.hpp"
#include "Sound.hpp"
#include "Timer.hpp"
#include "Serializer.hpp"

class Gameboy {
    Logger* log;
    Bus bus;
    Gpu gpu;
    Cpu cpu;
    Timer timer;
    Joypad joypad;
    Serial serial;
    Sound sound;
    long currentCycle;

public:
    Gameboy(Logger* log, Rom* rom, bool gbc) :
            log(log),
            bus(log, rom, &gpu, &timer, &joypad, &serial, &sound, gbc),
            gpu(log, &bus),
            cpu(log, &bus),
            timer(),
            joypad(),
            serial(),
            sound(log),
            currentCycle(0) {
    }


    Bus* getBus() { return &bus; }
    Cpu* getCpu() { return &cpu; }
    Gpu* getGpu() { return &gpu; }
    Timer* getTimer() { return &timer; }
    Joypad* getJoypad() { return &joypad; }
    Sound* getSound() { return &sound; }

    void serialize(Serializer& s);
    void runOneInstruction();
};
