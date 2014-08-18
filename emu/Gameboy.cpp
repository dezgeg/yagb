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

        if (joypad.tick())
            bus.raiseIrq(Irq_Joypad);
        int cycleDelta = cpu.tick();
        bus.tickDma(cycleDelta);
        IrqSet gpuIrqs = gpu.tick(cycleDelta);
        bus.raiseIrq(gpuIrqs);
        currentCycle += cycleDelta;
    }
}
