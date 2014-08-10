#include "Gameboy.hpp"
#include "Utils.hpp"

void Gameboy::runFrame()
{
    long frame = gpu.getCurrentFrame();
    log->warn("Start of frame %ld", frame);
    while (true) {
        long newFrame = gpu.getCurrentFrame();
        log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);
        if (newFrame != frame)
            return;

        int cycleDelta = cpu.tick();
        gpu.tick(cycleDelta);
        currentCycle += cycleDelta;
    }
}
