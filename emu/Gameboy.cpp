#include "Gameboy.hpp"
#include "Utils.hpp"

void Gameboy::run()
{
    while (!cpu.isHalted()) {
        log->setTimestamp(currentCycle, gpu.getCurrentScanline());

        int cycleDelta = cpu.tick();
        gpu.tick(cycleDelta);
        currentCycle += cycleDelta;
    }
}
