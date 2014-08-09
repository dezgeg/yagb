#include "Gameboy.hpp"
#include "Rom.hpp"

#include <stdio.h>

int main()
{
    Logger log;
    Rom rom(&log, "test.bin");
    Gameboy gb(&log, &rom);
    gb.run();
}
