Yet Another GameBoy Emulator
===

What?
---
Emulates the GameBoy (monochrome only) pretty decently.
All major features are implemented and plenty of games and demoscene releases run.

Supported features
---
- GameBoy CPU core (Z80-like)
- LCD controller
    - Tiled primary background & the window (a secondary BG layer)
    - Background scrolling
    - IRQs for VBlank & raster position
    - Sprites & sprite DMA
- Sound
    - 2 square wave channels
    - Wave channel
    - Noise channel
- Joypad (+ IRQ)
- Timer (+ IRQ)
- Dummy serial port (+ IRQ)
    - Some weird games use the transmit IRQ for timing purposes...
- MMC memory mappers + battery-backed save RAM 
- VRAM debug viewer
- Save states

What's missing
---
- Frequency sweep on the 1st square wave channel
- Real Time Clock in the MMC3
- Non-Nintendo mappers

And of course, plenty of bugs/emulation inaccuracies, see GitHub issues for some details.

Compiling & running
---
You'll need a C++11 compiler, Qt5 headers, and an OpenGL-capable machine. Compile with `make`.
Run with:
````
./yagb [-t] path/to/rom/file.gb
````

Use `-t` to print a trace of all executed instructions and memory accesses made by the CPU or sprite DMA. For example:

````
[mem rd (CPU)] 0x0100: 00
[insn 00332/153/00000404] 0x0100:       00 => NOP                              A: 0x01 | BC: 0x0013 | DE: 0x00d8 | HL: 0x014d | SP: 0xfffe | Flags: Z-HC. | Cycles: 4
[mem rd (CPU)] 0x0101: c3
[mem rd (CPU)] 0x0102: 50
[mem rd (CPU)] 0x0103: 01
[insn 00332/153/00000408] 0x0101: C3 50 01 => JP 0x0150                        A: 0x01 | BC: 0x0013 | DE: 0x00d8 | HL: 0x014d | SP: 0xfffe | Flags: Z-HC. | Cycles: 16
[mem rd (CPU)] 0x0150: f3
[insn 00332/153/00000424] 0x0150:       F3 => DI                               A: 0x01 | BC: 0x0013 | DE: 0x00d8 | HL: 0x014d | SP: 0xfffe | Flags: Z-HC. | Cycles: 4
````
