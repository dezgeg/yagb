#include "Gameboy.hpp"
#include "Utils.hpp"

void Gameboy::runFrame()
{
    long frame = gpu.getCurrentFrame();
    while (true) {
        long newFrame = gpu.getCurrentFrame();
        log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);
        if (newFrame != frame)
            return;

        int cycleDelta = cpu.tick();
        Irq gpuIrq = gpu.tick(cycleDelta);
        if (gpuIrq != Irq_None)
            bus.raiseIrq(gpuIrq);
        currentCycle += cycleDelta;
    }
}
