#pragma once
#include "Bus.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

union Regs
{
    struct {
        Byte c, b, e, d, l, h, _pad1, _pad2, f, a, _pad3, _pad4;
        Byte irqsEnabled : 1;
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

    bool evalConditional(Byte opc, char* outDescr, const char* opcodeStr);

    // ALU helpers
    Byte doAddSub(unsigned lhs, unsigned rhs, bool isSub, bool withCarry, bool updateCarry);
    Word doAdd16(unsigned lhs, unsigned rhs);
    Byte doRotLeft(Byte v);
    Byte doRotLeftWithCarry(Byte v);
    Byte doRotRight(Byte v);
    Byte doRotRightWithCarry(Byte v);
    Byte doAluOp(int aluop, Byte lhs, Byte rhs);

    long executeInsn_0x_3x(Byte opc);
    long executeInsn_4x_6x(Byte opc);
    long executeInsn_7x_Bx(Byte opc);
    long executeInsn_Cx_Fx(Byte opc);
    long executeTwoByteInsn();

public:
    Cpu(Logger* log, Bus* bus) :
        log(log),
        bus(bus)
    {
        reset();
    }

    bool isHalted() { return halted; }
    Regs* getRegs() { return &regs; }

    void reset();
    long tick();
};
