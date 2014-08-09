#pragma once
#include "Bus.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

union Regs
{
    struct {
        Byte c, b, e, d, l, h, _pad1, _pad2, f, a;
    };
    struct {
        Word bc, de, hl, sp, af, pc;
    };
    struct {
        Byte _pad[8];
        Byte unimplemented : 4;
        Byte c : 1;     // Carry
        Byte h : 1;     // Half-carry
        Byte n : 1;     // Add/Sub (for BCD operations)
        Byte z : 1;     // Zero
    } flags;
    Byte bytes[8];
    Word words[6];
};

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error "You are using piss-poor hardware and are a loser; give up and get a real job."
#endif

class Gameboy;
class Cpu
{
    Logger* log;
    Bus* bus;

    Regs regs;
    bool halted;
    bool stopped;
    bool interruptsEnabled;

    bool evalConditional(Byte opc, char* outDescr, const char* opcodeStr);

    // ALU helpers
    Byte doAddSub(unsigned lhs, unsigned rhs, bool isSub, bool withCarry, bool updateCarry);
    Byte doRotLeft(Byte v);
    Byte doRotLeftWithCarry(Byte v);
    Byte doRotRight(Byte v);
    Byte doRotRightWithCarry(Byte v);

    void executeInsn_0x_3x(Byte opc);
    void executeInsn_4x_6x(Byte opc);
    void executeInsn_7x_Bx(Byte opc);
    void executeInsn_Cx_Fx(Byte opc);
    void executeTwoByteInsn();

public:
    Cpu(Logger* log, Bus* bus) :
        log(log),
        bus(bus)
    {
        reset();
    }

    bool isHalted() { return halted; }

    void reset();
    void tick();
};
