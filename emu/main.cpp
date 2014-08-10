#include "Gameboy.hpp"
#include "Rom.hpp"

#include <stdio.h>
#include <unistd.h>

int main()
{
    Logger log;
    Rom rom(&log, "test.bin");
    Gameboy gb(&log, &rom);

    while (true) {
        gb.runFrame();

#if 1
        Gpu* gpu = gb.getGpu();
        Byte* framebuffer = gpu->getFramebuffer();
        // clear screen
        // printf("\x1b\x5b\x48\x1b\x5b\x32\x4a");
        printf("\n\n\n\n\n");
        fflush(stdout);

        for (long i = 0; i < ScreenHeight; i++) {
            for (long j = 0; j < ScreenWidth; j++)
                putchar(framebuffer[ScreenHeight * i + j] + '0');
            putchar('\n');
        }
        usleep(20000);
#endif
    }
}
