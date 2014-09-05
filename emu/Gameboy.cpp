#include "Gameboy.hpp"

void Gameboy::runOneInstruction() {
    long newFrame = gpu.getCurrentFrame();
    log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);

    if (joypad.tick()) {
        bus.raiseIrq(bit(Irq_Joypad));
    }

    int cycleDelta = cpu.tick();

    bus.tickDma(cycleDelta);
    if (timer.tick(cycleDelta)) {
        bus.raiseIrq(bit(Irq_Timer));
    }
    if (serial.tick(cycleDelta)) {
        bus.raiseIrq(bit(Irq_Serial));
    }

    IrqSet gpuIrqs = gpu.tick(cycleDelta);
    bus.raiseIrq(gpuIrqs);
    sound.tick(cycleDelta);
    currentCycle += cycleDelta;
}
