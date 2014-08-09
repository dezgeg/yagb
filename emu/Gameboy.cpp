#include "Gameboy.hpp"
#include "Utils.hpp"

void Gameboy::run()
{
    while (!cpu.isHalted())
        cpu.tick();
}
