#include "Gameboy.hpp"

#include <stdio.h>

int main()
{
    Logger log;
    Gameboy gb(&log, "test.bin");
    gb.run();
}
