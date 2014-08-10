#include "Gameboy.hpp"
#include "Utils.hpp"

void Gameboy::run()
{
    long frame = -1;
    while (!cpu.isHalted()) {
        long newFrame = gpu.getCurrentFrame();
        log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);
        if (newFrame != frame) {
            frame = newFrame;
            log->warn("Start of frame %ld", frame);
        }

        int cycleDelta = cpu.tick();
        gpu.tick(cycleDelta);
        currentCycle += cycleDelta;
    }
}
