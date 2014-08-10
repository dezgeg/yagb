#include "Gameboy.hpp"
#include "Utils.hpp"

void Gameboy::run()
{
    while (!cpu.isHalted()) {
        log->setCycle(currentCycle);
        currentCycle += cpu.tick();
    }
}
